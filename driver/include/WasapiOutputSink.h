#pragma once

#include "StereoRingBuffer.h"

#include <RtAudio.h>

#include <memory>
#include <string>
#include <atomic>

class WasapiOutputSink
{
public:
    WasapiOutputSink();
    ~WasapiOutputSink();

    bool start(
        StereoRingBuffer* ringBuffer,
        unsigned int sampleRate,
        unsigned int bufferFrames,
        float outputGain,
        bool useDefaultDevice,
        const std::string& preferredDeviceName);

    void stop();

    bool isRunning() const;
    const std::string& lastError() const;
    void setOutputGain(float outputGain);

private:
    static int audioCallback(
        void* outputBuffer,
        void* inputBuffer,
        unsigned int nBufferFrames,
        double streamTime,
        RtAudioStreamStatus status,
        void* userData);

    int render(
        void* outputBuffer,
        unsigned int nBufferFrames,
        RtAudioStreamStatus status);

    std::unique_ptr<RtAudio> audio_;
    StereoRingBuffer* ringBuffer_ = nullptr;

    bool running_ = false;
    std::string lastError_;
    std::atomic<float> outputGain_{1.0f};
    unsigned int findOutputDevice() const;

    bool useDefaultDevice_ = true;
    std::string preferredDeviceName_;
};