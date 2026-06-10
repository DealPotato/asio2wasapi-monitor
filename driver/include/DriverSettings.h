#pragma once

namespace DriverSettings
{
    inline constexpr bool EnableDebugLogging = true;

    inline constexpr bool EnableTestInputTone = false;

    inline constexpr unsigned int DefaultSampleRate = 48000;
    inline constexpr unsigned int AsioBufferFrames = 128;
    inline constexpr unsigned int WasapiBufferFrames = 256;

    inline constexpr unsigned int OutputRingFrames = 2048;
    inline constexpr unsigned int InputRingFrames = 2048;

    // Scarlett Input 2 = zero-based channel 1.
    inline constexpr unsigned int HardwareInputChannel = 1;

    inline constexpr const char* PreferredAsioInputName1 = "Focusrite";
    inline constexpr const char* PreferredAsioInputName2 = "Scarlett";
}