#pragma once

#include "DriverSettings.h"

#include <string>
#include <filesystem>

struct DriverConfig
{
    unsigned int sampleRate = DriverSettings::DefaultSampleRate;
    unsigned int wasapiBufferFrames = DriverSettings::WasapiBufferFrames;

    unsigned int inputRingFrames = DriverSettings::InputRingFrames;
    unsigned int outputRingFrames = DriverSettings::OutputRingFrames;

    std::string preferredAsioInputDevice = DriverSettings::PreferredAsioInputName1;
    unsigned int hardwareInputChannel = DriverSettings::HardwareInputChannel;

    float inputGain = 1.0f;
    float outputGain = 1.0f;
    bool useDefaultWasapiDevice = true;
    std::string preferredWasapiDevice;

    bool enableTestTone = DriverSettings::EnableTestInputTone;
    bool enableLogging = DriverSettings::EnableDebugLogging;

    static DriverConfig load();
    static std::filesystem::path configPath();
    static std::filesystem::file_time_type lastWriteTime();
    
};