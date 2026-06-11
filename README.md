# ASIO2WASAPI Monitor

**ASIO2WASAPI Monitor** is an experimental Windows x64 virtual ASIO driver and control panel designed to route hardware ASIO input through an ASIO host/DAW and monitor the processed output through a WASAPI output device.

The main use case is:

```text
Guitar / instrument
→ Hardware ASIO input device
→ ASIO2WASAPI Virtual ASIO input
→ DAW / plugin host / amp simulator
→ ASIO2WASAPI Virtual ASIO output
→ WASAPI headphones / speakers
```

This allows using an ASIO input device, such as a Focusrite Scarlett interface, while monitoring through a regular Windows audio output device.

## Current Status

This project is currently in an **experimental v0.1.0** stage.

It is functional, but still early. The driver has been tested primarily with:

```text
Windows 11 x64
REAPER
Focusrite USB ASIO
WASAPI headphones
```

Other DAWs, audio interfaces, and WASAPI output devices may work, but are not guaranteed yet.

## Features

* Virtual ASIO driver for Windows x64.
* Hardware ASIO input capture.
* WASAPI output monitoring.
* ASIO host support through standard ASIO driver registration.
* Control panel GUI.
* Install and uninstall buttons inside the control panel.
* Preferred ASIO input device selection.
* Preferred WASAPI output device selection.
* Input channel selection.
* Runtime configuration through `asio2wasapi-monitor.ini`.
* Live reload for most internal settings.
* Input and output gain controls.
* Adjustable WASAPI buffer size.
* Adjustable input and output ring buffer sizes.
* Test tone option.
* Debug logging option.
* ASIO Configuration button support from DAWs such as REAPER.

## Included Files

A release package should contain:

```text
asio2wasapi-virtual-asio.dll
rtaudio.dll
asio2wasapi-control.exe
asio2wasapi-devices.exe
asio2wasapi-monitor.ini
README.md
LICENSE
```

## Quick Start

1. Extract the release ZIP to a permanent folder.

   Example:

   ```text
   C:\Tools\asio2wasapi-monitor
   ```

2. Run:

   ```text
   asio2wasapi-control.exe
   ```

3. Click:

   ```text
   Install Driver
   ```

   Administrator permission is required because the ASIO driver must be registered in the Windows Registry.

4. Configure the driver:

   Recommended starting settings:

   ```text
   Sample Rate: 48000
   ASIO Buffer Frames: 128
   WASAPI Buffer Frames: 256
   Input Ring Frames: 2048
   Output Ring Frames: 2048
   Preferred ASIO Input Device: Focusrite
   Hardware Input Channel: 1
   Use Windows Default WASAPI Output Device: On
   Input Gain: 1.00
   Output Gain: 1.00
   Enable Test Tone: Off
   Enable Debug Logging: On
   ```

5. Open your DAW or ASIO host.

6. Select:

   ```text
   ASIO2WASAPI Virtual ASIO
   ```

7. In the DAW, create an input-monitored track.

   Example for REAPER:

   ```text
   Audio System: ASIO
   ASIO Driver: ASIO2WASAPI Virtual ASIO
   Track Input: Mono Input 1
   Record Arm: On
   Record Monitoring: On
   ```

8. Play your instrument and monitor the processed output through your selected WASAPI output device.

## Control Panel

The control panel can be opened directly:

```text
asio2wasapi-control.exe
```

It can also be opened from the DAW using:

```text
ASIO Configuration...
```

The control panel supports:

```text
Install Driver
Uninstall Driver
Save / Apply Live
Reload
Open Config Folder
```

Most internal settings are reloaded automatically by the running driver within a few seconds.

Some host-controlled settings, such as sample rate and host ASIO buffer size, may still require reselecting the driver in the DAW.

## Runtime Configuration

The driver reads settings from:

```text
asio2wasapi-monitor.ini
```

Example configuration:

```ini
[Audio]
sampleRate=48000
asioBufferFrames=128
wasapiBufferFrames=256
inputRingFrames=2048
outputRingFrames=2048

[Input]
preferredAsioInputDevice=Focusrite
hardwareInputChannel=1
inputGain=1.0
enableTestTone=false

[Output]
useDefaultWasapiDevice=true
preferredWasapiDevice=
outputGain=1.0

[Debug]
enableLogging=true
```

## Hardware Input Channel

The hardware input channel is zero-based.

Examples:

```text
0 = Input 1
1 = Input 2
2 = Input 3
```

For example, if a guitar is connected to Input 2 on a Focusrite Scarlett interface, use:

```ini
hardwareInputChannel=1
```

## Latency Presets

Recommended stable preset:

```ini
[Audio]
sampleRate=48000
asioBufferFrames=128
wasapiBufferFrames=256
inputRingFrames=2048
outputRingFrames=2048
```

Low latency preset:

```ini
[Audio]
sampleRate=48000
asioBufferFrames=128
wasapiBufferFrames=128
inputRingFrames=1024
outputRingFrames=1024
```

Ultra low latency experimental preset:

```ini
[Audio]
sampleRate=48000
asioBufferFrames=64
wasapiBufferFrames=128
inputRingFrames=512
outputRingFrames=512
```

If audio crackles, drops out, or becomes unstable, increase the WASAPI buffer and ring buffer sizes.

## Debug Log

When debug logging is enabled, the driver writes logs to:

```text
%TEMP%\asio2wasapi-driver.log
```

Useful PowerShell command:

```powershell
Get-Content "$env:TEMP\asio2wasapi-driver.log" -Tail 120
```

Useful log entries include:

```text
ASIO input started
WASAPI output started
Runtime config file changed, applying settings
Runtime config applied
inputUnder
inputDrop
outputUnder
outputDrop
```

If drop or underrun counters keep increasing, the selected latency settings may be too aggressive.

## Build From Source

Requirements:

```text
Windows 10/11 x64
Visual Studio 2022
CMake
.NET 8 SDK
Git
```

Clone with submodules:

```powershell
git clone --recursive <repository-url>
cd asio2wasapi-monitor
```

Configure x64 build:

```powershell
cmake -S . -B build-x64 -G "Visual Studio 17 2022" -A x64
```

Build C++ targets:

```powershell
cmake --build build-x64 --config Release
```

Build/publish the control panel:

```powershell
dotnet publish .\tools\Asio2Wasapi.ControlPanel -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true
```

Copy runtime files:

```powershell
New-Item -ItemType Directory -Path .\installed-driver -Force | Out-Null

Copy-Item ".\build-x64\Release\asio2wasapi-virtual-asio.dll" ".\installed-driver\" -Force
Copy-Item ".\build-x64\external\rtaudio\Release\rtaudio.dll" ".\installed-driver\" -Force
Copy-Item ".\build-x64\Release\asio2wasapi-devices.exe" ".\installed-driver\" -Force
Copy-Item ".\tools\Asio2Wasapi.ControlPanel\bin\Release\net8.0-windows\win-x64\publish\asio2wasapi-control.exe" ".\installed-driver\" -Force
```

Then run:

```powershell
.\installed-driver\asio2wasapi-control.exe
```

## Manual Driver Registration

The recommended method is to use the control panel.

Manual registration can also be done with the included PowerShell script:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

.\scripts\register-driver.ps1 -DllPath "C:\Path\To\asio2wasapi-virtual-asio.dll"
```

Manual unregister:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

.\scripts\unregister-driver.ps1
```

## Troubleshooting

### The driver does not appear in my DAW

Make sure the driver is installed using the control panel.

Also confirm that the application is running as x64. This project currently targets Windows x64.

### ASIO Configuration does not open the control panel

Make sure this file is next to the driver DLL:

```text
asio2wasapi-control.exe
```

The driver looks for the control panel in the same folder as:

```text
asio2wasapi-virtual-asio.dll
```

### No sound

Check:

```text
The ASIO input device is correct.
The hardware input channel is correct.
Record monitoring is enabled in the DAW.
The WASAPI output device is correct.
Input Gain and Output Gain are not set to 0.
Enable Test Tone is off for normal guitar/instrument use.
```

For Focusrite Scarlett Input 2, use:

```ini
hardwareInputChannel=1
```

### Audio crackles or drops out

Try increasing:

```text
WASAPI Buffer Frames
Input Ring Frames
Output Ring Frames
```

Recommended fallback:

```ini
wasapiBufferFrames=256
inputRingFrames=2048
outputRingFrames=2048
```

### Settings do not apply immediately

Most internal settings reload live within a few seconds.

However, some host-controlled settings may require reselecting the ASIO driver in the DAW:

```text
Sample rate
Host ASIO buffer size
```

## Known Limitations

* Experimental first release.
* Windows x64 only.
* Tested mainly with REAPER and Focusrite USB ASIO.
* Some DAWs may behave differently.
* Sample rate and host ASIO buffer handling may depend on the ASIO host.
* No installer package yet.
* Driver is not code-signed.
* Device names are matched by text.
* WASAPI exclusive/shared behavior may depend on the selected Windows device and system settings.

## License

This project is licensed under the GPLv3 license.

See `LICENSE` for details.
