# ASIO2WASAPI Monitor

Experimental Windows x64 virtual ASIO driver and control panel for routing a hardware ASIO input through a DAW/plugin host and monitoring the processed output through a WASAPI device.

Typical use case:

```text
Guitar -> Scarlett ASIO input -> ASIO2WASAPI Virtual ASIO -> REAPER / amp sim -> WASAPI headphones
```

> Status: experimental. Tested primarily on Windows 11 x64 with REAPER, Focusrite USB ASIO and WASAPI headphones.

## What is included

- `asio2wasapi-virtual-asio.dll` - virtual ASIO driver.
- `asio2wasapi-control.exe` - dark themed control panel for devices, presets and driver install/uninstall.
- `asio2wasapi-devices.exe` - helper used by the control panel to list ASIO/WASAPI devices.
- `rtaudio.dll` - RtAudio runtime dependency.
- `asio2wasapi-monitor.ini` - runtime configuration next to the driver DLL.

## Current features

- Virtual ASIO driver visible to ASIO hosts.
- Hardware ASIO input capture through RtAudio.
- WASAPI output sink with shared/exclusive mode option.
- MMCSS `Pro Audio` callback thread priority.
- Device scanner for ASIO input and WASAPI output devices.
- Control panel with latency presets:
  - Safe
  - Balanced
  - Low Latency
  - Experimental
- Debug logging can be enabled only when needed; it is off by default for better realtime behavior.

## Quick start

1. Build the runtime files:

   ```powershell
   cmake --build build-x64 --config Release --target install-local
   ```

2. Open the control panel:

   ```powershell
   .\installed-driver\asio2wasapi-control.exe
   ```

3. Click **Install Driver**.
4. Choose your hardware ASIO input device, input channel and WASAPI output device.
5. Start with the **Balanced** preset.
6. Save settings.
7. In your DAW/plugin host, select **ASIO2WASAPI Virtual ASIO** as the ASIO driver.

## Recommended starting settings

```ini
[Audio]
sampleRate=48000
asioBufferFrames=128
wasapiBufferFrames=128
inputRingFrames=1024
outputRingFrames=1024

[Input]
preferredAsioInputDevice=Focusrite
hardwareInputChannel=1
inputGain=1.0
enableTestTone=false

[Output]
useDefaultWasapiDevice=true
preferredWasapiDevice=
wasapiExclusiveMode=true
outputGain=1.0

[Debug]
enableLogging=false
```

For a Scarlett with the guitar plugged into Input 2, use `hardwareInputChannel=1` because the value is zero-based.

## Latency notes

The main latency cost is not C++ execution time. It comes from ASIO buffers, WASAPI buffers, ring-buffer safety depth, host/plugin processing and Windows scheduling.

The safest low-latency path is:

1. Keep logging off while playing.
2. Use WASAPI exclusive mode when the output device supports it.
3. Start with `wasapiBufferFrames=128`, `inputRingFrames=1024`, `outputRingFrames=1024`.
4. Try `outputRingFrames=768` only after the stable preset is clean.
5. Avoid `512` output safety unless your system is completely stable.

## Debug log

Debug logging is intentionally disabled by default. Enable it in the control panel only while troubleshooting, then restart/reselect the ASIO driver in the host.

The log file is written to:

```powershell
$env:TEMP\asio2wasapi-driver.log
```

Useful command:

```powershell
Get-Content "$env:TEMP\asio2wasapi-driver.log" -Tail 120
```

Live follow:

```powershell
Get-Content "$env:TEMP\asio2wasapi-driver.log" -Wait -Tail 80
```

## Build from source

Requirements:

- Windows x64
- Visual Studio 2022 with C++ desktop workload
- CMake 3.24+
- .NET 8 SDK
- RtAudio submodule/source present under `external/rtaudio`

Configure and build:

```powershell
cmake -S . -B build-x64 -A x64
cmake --build build-x64 --config Release --target install-local
```

The local runtime output is copied to:

```text
installed-driver/
```

## Troubleshooting

### The DLL will not copy during build

Close the host and control panel first:

```powershell
taskkill /IM reaper.exe /F
taskkill /IM asio2wasapi-control.exe /F
taskkill /IM asio2wasapi-devices.exe /F
```

Then rebuild.

### Crackles at low buffers

Use the Balanced preset first. If 768 or 512 output safety buffers crackle, return to 1024. Clean monitoring is more important than an unusable lower number.

### Noise when using an amp sim

Bypass the amp sim first. High-gain amp sims can magnify guitar wiring, cable, grounding, inactive-input and gain-stage noise. Verify the guitar is silent with the volume at zero before blaming the driver.

## Limitations

- Windows x64 only.
- Experimental virtual ASIO driver.
- Configuration changes generally require reselecting/restarting the driver in the host.
- The control panel does not yet display live driver metrics directly; log-based diagnostics are used for now.
- Lower latency targets still need deeper work such as lock-free SPSC ring buffers and improved callback timing.

## License

GPLv3. See [LICENSE](LICENSE).
