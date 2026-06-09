# Virtual ASIO Driver Research

## Goal

The goal is to investigate whether ASIO2WASAPI Monitor can support DAW integration through a virtual ASIO driver.

Current standalone mode:

```text
Guitar → Scarlett ASIO → ASIO2WASAPI Monitor → WASAPI Headphones
```

Possible future DAW mode:

```text
DAW → ASIO2WASAPI Virtual ASIO Driver → WASAPI Output
```

## Why a Virtual ASIO Driver?

The current application works as a standalone bridge, but it does not appear inside DAWs as an ASIO device.

A virtual ASIO driver would allow DAWs to select ASIO2WASAPI Monitor as an audio driver.

Potential benefits:

- DAW integration
- More control over reported buffer sizes
- Custom channel layout
- Custom routing
- Possible reduction of unnecessary buffering
- Better control over realtime audio flow

## Important Limitation

A virtual ASIO driver cannot remove all latency.

Total latency may still include:

- DAW processing buffer
- Driver buffer
- WASAPI output buffer
- Output device latency
- Wireless headset latency

## Initial Architecture

```text
DAW
  ↓
ASIO2WASAPI Virtual ASIO Driver DLL
  ↓
Shared audio buffer / IPC layer
  ↓
ASIO2WASAPI Bridge Core
  ↓
WASAPI Output Device
```

## Milestone 1: Driver Skeleton

The first goal is not audio playback.

The first goal is:

- Build a virtual ASIO driver DLL
- Register it on Windows
- Make it appear in a DAW or ASIO host
- Let the host query basic driver information
- Avoid crashing when selected

Success criteria:

- The driver appears in at least one ASIO host
- The host can select the driver
- The driver reports basic information such as name, version, channel count, sample rate, and buffer sizes
- The host does not crash when the driver is selected

## Milestone 2: Silent Callback

The second goal is to make the driver participate in the ASIO callback flow without producing real audio.

The driver should:

- Receive buffer callbacks from the host
- Output silence
- Log callback activity for debugging
- Correctly handle start and stop
- Correctly handle buffer creation and disposal

Possible test flow:

```text
DAW selects ASIO2WASAPI Virtual ASIO Driver
  ↓
DAW creates ASIO buffers
  ↓
DAW starts playback
  ↓
Driver receives callbacks
  ↓
Driver writes silence
```

## Milestone 3: DAW Output to WASAPI

The third goal is actual audio output from the DAW to a WASAPI device.

Target flow:

```text
DAW output buffer
  ↓
ASIO2WASAPI Virtual ASIO Driver
  ↓
Shared buffer or direct WASAPI render path
  ↓
WASAPI headphones
```

This would allow a DAW to output sound through a WASAPI device while still using an ASIO driver from the DAW's point of view.

## Milestone 4: Hardware Input to DAW

A later goal is exposing hardware input to the DAW.

Target flow:

```text
Scarlett ASIO input
  ↓
ASIO2WASAPI bridge layer
  ↓
ASIO2WASAPI Virtual ASIO Driver
  ↓
DAW input
```

This is more complex because the driver would need to coordinate hardware ASIO input timing with the DAW's ASIO callback timing.

## Milestone 5: Full Duplex DAW Mode

Long-term target:

```text
Scarlett ASIO input
  ↓
ASIO2WASAPI Virtual ASIO Driver
  ↓
DAW processing
  ↓
ASIO2WASAPI Virtual ASIO Driver
  ↓
WASAPI headphones
```

This would make the virtual driver act as a full DAW audio interface layer.

## Possible Project Structure

```text
asio2wasapi-monitor/
├─ src/
│  ├─ standalone app
│  ├─ bridge core
│  └─ device utilities
├─ driver/
│  ├─ virtual ASIO driver DLL
│  ├─ driver registration code
│  └─ ASIO host callback implementation
├─ shared/
│  ├─ shared audio buffer
│  ├─ shared configuration
│  └─ common realtime audio utilities
└─ docs/
   └─ virtual-asio-driver.md
```

## Driver and App Separation

The virtual ASIO driver should probably not contain all application logic.

Preferred architecture:

```text
ASIO driver DLL:
- Loaded by the DAW
- Exposes ASIO driver interface
- Handles ASIO callbacks
- Communicates with bridge core
- Must be stable and lightweight

Bridge app/service:
- Owns user configuration
- Owns WASAPI output selection
- Owns monitoring UI
- May manage shared memory or IPC
```

Reason:

- If the driver crashes, the DAW may crash
- The driver should do as little work as possible
- Heavy UI or configuration logic should stay outside the driver
- Realtime callback code should avoid locks, allocations, blocking calls, and logging inside the callback path

## Communication Options

### Option A: Shared Memory

Possible flow:

```text
Virtual ASIO Driver DLL
  ↓
Shared memory ring buffer
  ↓
Bridge app
  ↓
WASAPI output
```

Pros:

- Fast
- Suitable for audio buffers
- Can avoid unnecessary copying if designed carefully

Cons:

- More complex synchronization
- Needs careful lifetime management
- Must handle app/driver startup order
- Must handle crashes or disconnects

### Option B: Driver Manages WASAPI Directly

Possible flow:

```text
DAW
  ↓
Virtual ASIO Driver DLL
  ↓
WASAPI output
```

Pros:

- Simpler routing path
- Potentially lower latency
- No separate bridge app needed for basic playback

Cons:

- Driver becomes more complex
- Device selection and UI become harder
- WASAPI initialization inside a DAW-loaded driver may be fragile
- More risk of crashing the host

### Option C: Existing App as Audio Engine

Possible flow:

```text
DAW
  ↓
Virtual ASIO Driver DLL
  ↓
IPC/shared buffer
  ↓
ASIO2WASAPI Monitor app
  ↓
WASAPI output
```

Pros:

- Keeps the driver minimal
- Allows GUI and configuration in the app
- Easier to debug app separately

Cons:

- Adds another layer
- May add latency
- Requires robust synchronization

## Realtime Safety Rules

The ASIO callback path should avoid:

- Memory allocation
- File I/O
- Console output
- Locks/mutexes where possible
- Blocking IPC
- Waiting on events
- Complex device enumeration
- GUI calls

The callback path should ideally only:

- Read/write audio buffers
- Move read/write indices
- Update atomic counters
- Output silence on underrun
- Return quickly

## Open Questions

- Should the driver output to WASAPI directly, or should a separate bridge app handle WASAPI?
- Should audio be transferred through shared memory?
- How should the app and driver discover each other?
- What happens if the DAW starts before the bridge app?
- What happens if the bridge app closes while the DAW is running?
- How should clock drift be handled?
- What channel count should be exposed to the DAW?
- What buffer sizes should be reported?
- Which sample rates should be supported initially?
- Should the first version be output-only?
- Should input support come later?
- How should driver registration and unregistration be handled?
- How should logs be collected without violating realtime safety?
- How should crashes be isolated from the DAW?

## Initial Recommendation

Start with the smallest possible virtual ASIO driver skeleton.

Do not attempt full audio routing immediately.

Recommended implementation order:

1. Build a virtual ASIO driver DLL
2. Register it on Windows
3. Make it appear in an ASIO host
4. Report basic driver information
5. Support buffer creation
6. Support start and stop
7. Output silence
8. Add debug counters
9. Route DAW output to WASAPI
10. Add hardware input support later

## Current Priority

Before implementing the virtual driver, keep the standalone ASIO to WASAPI bridge stable.

The current working standalone flow remains:

```text
Guitar → Focusrite USB ASIO → ASIO2WASAPI Monitor → WASAPI Headphones
```

The virtual ASIO driver should be developed as a separate layer and should not break the existing standalone bridge.