#pragma once

#include "MonoRingBuffer.h"

#include <RtAudio.h>

#include <memory>
#include <string>
#include <vector>

class AsioInputSource
{
public:
    AsioInputSource();
    ~AsioInputSource();

    bool start(
        MonoRingBuffer* ringBuffer,
        unsigned int sampleRate,
        unsigned int bufferFrames,
        unsigned int sourceChannel);

    void stop();

    bool isRunning() const;
    const std::string& lastError() const;

private:
    static int audioCallback(
        void* outputBuffer,
        void* inputBuffer,
        unsigned int nBufferFrames,
        double streamTime,
        RtAudioStreamStatus status,
        void* userData);

    int capture(
        void* inputBuffer,
        unsigned int nBufferFrames,
        RtAudioStreamStatus status);

    unsigned int findInputDevice() const;

    std::unique_ptr<RtAudio> audio_;
    MonoRingBuffer* ringBuffer_ = nullptr;

    unsigned int sourceChannel_ = 1;   // Scarlett input 2, zero-based.
    unsigned int openedChannels_ = 2;

    bool running_ = false;
    std::string lastError_;
    std::vector<float> captureScratch_;
};