#pragma once

#include "DriverDefaults.h"

#include <filesystem>
#include <string>

struct DriverConfig
{
    unsigned int sampleRate = DriverDefaults::DefaultSampleRate;
    unsigned int asioBufferFrames = DriverDefaults::AsioBufferFrames;
    unsigned int wasapiBufferFrames = DriverDefaults::WasapiBufferFrames;

    unsigned int inputRingFrames = DriverDefaults::InputRingFrames;
    unsigned int outputRingFrames = DriverDefaults::OutputRingFrames;

    std::string preferredAsioInputDevice = DriverDefaults::PreferredAsioInputDevice;
    unsigned int hardwareInputChannel = DriverDefaults::HardwareInputChannel;

    float inputGain = DriverDefaults::InputGain;
    bool enableTestTone = DriverDefaults::EnableTestInputTone;

    bool useDefaultWasapiDevice = DriverDefaults::UseDefaultWasapiDevice;
    std::string preferredWasapiDevice = DriverDefaults::PreferredWasapiDevice;
    bool wasapiExclusiveMode = DriverDefaults::WasapiExclusiveMode;

    float outputGain = DriverDefaults::OutputGain;

    bool enableLogging = DriverDefaults::EnableDebugLogging;

    static DriverConfig load();
    static std::filesystem::path configPath();
    static std::filesystem::file_time_type lastWriteTime();
};