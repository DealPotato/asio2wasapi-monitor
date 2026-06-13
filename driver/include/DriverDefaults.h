#pragma once

namespace DriverDefaults
{
    inline constexpr bool EnableDebugLogging = false;
    inline constexpr bool EnableTestInputTone = false;

    inline constexpr unsigned int DefaultSampleRate = 48000;
    inline constexpr unsigned int AsioBufferFrames = 128;
    inline constexpr unsigned int WasapiBufferFrames = 256;

    inline constexpr unsigned int InputRingFrames = 2048;
    inline constexpr unsigned int OutputRingFrames = 2048;

    inline constexpr unsigned int HardwareInputChannel = 1;

    inline constexpr float InputGain = 1.0f;
    inline constexpr float OutputGain = 1.0f;

    inline constexpr bool UseDefaultWasapiDevice = true;
    inline constexpr bool WasapiExclusiveMode = false;

    inline constexpr const char* PreferredAsioInputDevice = "Focusrite";
    inline constexpr const char* PreferredWasapiDevice = "";
}