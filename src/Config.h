#pragma once

#include <cstddef>

struct Config
{
    unsigned int asioDeviceId = 130;
    unsigned int wasapiDeviceId = 131;
    unsigned int inputChannel = 2;
    unsigned int sampleRate = 48000;
    unsigned int bufferFrames = 128;
    unsigned int prebufferMs = 5;
    float gain = 1.0f;
    std::size_t ringSamples = 8192;
    bool listDevices = false;
};

void printHelp();
Config parseArgs(int argc, char** argv);