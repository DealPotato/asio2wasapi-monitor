# ASIO2WASAPI Monitor

ASIO2WASAPI Monitor is an open-source Windows audio utility that routes low-latency ASIO input to a WASAPI output device.

The initial use case is direct guitar monitoring:
```text
Guitar → Audio Interface ASIO Input → ASIO2WASAPI Monitor → WASAPI Headphones

This allows an ASIO audio interface input, such as a Focusrite Scarlett guitar input, to be monitored through a separate Windows audio output device, such as USB or wireless headphones.

Current Status

This project is currently in early MVP stage.

Working features:

ASIO backend support
WASAPI backend support
ASIO input capture
WASAPI stereo output
Mono input to stereo output routing
Runtime configuration through command-line arguments
Smart prebuffer startup
Device listing mode
Input peak, ring buffer, underrun, and overrun monitoring

Planned features:

Graphical user interface
Device selection by name
Presets for low-latency and stable monitoring
Better latency tuning
Optional gain/mute controls
Future research into virtual ASIO driver support for DAW integration
Example Setup

Tested development chain:

Guitar
  ↓
Focusrite USB ASIO input
  ↓
ASIO2WASAPI Monitor
  ↓
Arctis 7+ WASAPI headphones

Example devices:

ASIO input:
[130] Focusrite USB ASIO

WASAPI output:
[131] Headphones (3- Arctis 7+)

Your device IDs may be different. Use --list to find the correct IDs on your system.

Usage

List available ASIO and WASAPI devices:

.\asio2wasapi-monitor.exe --list

Start the bridge:

.\asio2wasapi-monitor.exe --asio 130 --wasapi 131 --channel 2 --buffer 64 --prebuffer 1 --gain 1.0
Command-Line Options
--list               List ASIO and WASAPI devices, then exit
--asio <id>          ASIO input device ID
--wasapi <id>        WASAPI output device ID
--channel <n>        ASIO input channel, 1-based
--rate <hz>          Sample rate
--buffer <frames>    Buffer size in frames
--prebuffer <ms>     Startup prebuffer in milliseconds
--gain <value>       Output gain
--ring <samples>     Ring buffer size in samples
--help               Show help
Low-Latency Example
.\asio2wasapi-monitor.exe --asio 130 --wasapi 131 --channel 2 --buffer 64 --prebuffer 1 --gain 1.0

If you hear crackles, dropouts, or underruns, try a safer setting:

.\asio2wasapi-monitor.exe --asio 130 --wasapi 131 --channel 2 --buffer 128 --prebuffer 2 --gain 1.0
Notes on Latency

Zero latency is not physically possible. The total perceived latency depends on:

ASIO input buffer size
Ring buffer fill level
WASAPI output buffer size
Prebuffer setting
The output device latency
Wireless headset or USB audio latency

Lower buffer and prebuffer values reduce latency but may increase the risk of underruns or audio glitches.

DAW Integration

The current version is a standalone monitor application.

It does not yet appear as an ASIO driver inside DAWs.

Future DAW support would require a virtual ASIO driver layer, separate from the current standalone bridge.

Possible future architecture:

DAW
  ↓
ASIO2WASAPI Virtual ASIO Driver
  ↓
ASIO2WASAPI Bridge/App
  ↓
WASAPI Output Device

This is planned as a research direction, not part of the current MVP.

Build

This project uses CMake and RtAudio.

Basic build flow:

cmake -S . -B build
cmake --build build --config Release

The executable will be generated under:

build\Release\
License

This project is licensed under the GNU General Public License v3.0.

Project Status

ASIO2WASAPI Monitor is experimental software.

It is currently intended for learning, testing, and open-source development.
