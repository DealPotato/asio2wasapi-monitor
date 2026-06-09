#pragma once

#include "Config.h"
#include "MonoRingBuffer.h"

#include <RtAudio.h>

#include <atomic>
#include <cstddef>
#include <memory>

struct BridgeState
{
    explicit BridgeState(const Config& cfg)
        : config(cfg), ring(cfg.ringSamples)
    {
    }

    Config config;
    MonoRingBuffer ring;

    std::atomic<float> inputPeak{0.0f};
    std::atomic<unsigned long long> inputCallbacks{0};
    std::atomic<unsigned long long> outputCallbacks{0};
    std::atomic<unsigned long long> underruns{0};
    std::atomic<unsigned long long> overruns{0};
};

class AudioBridge
{
public:
    explicit AudioBridge(const Config& config);
    ~AudioBridge();

    AudioBridge(const AudioBridge&) = delete;
    AudioBridge& operator=(const AudioBridge&) = delete;

    void start();
    void stop();

    bool isRunning() const;

    float inputPeak() const;
    std::size_t ringAvailable() const;
    unsigned long long inputCallbacks() const;
    unsigned long long outputCallbacks() const;
    unsigned long long underruns() const;
    unsigned long long overruns() const;

private:
    static int asioInputCallback(
        void* outputBuffer,
        void* inputBuffer,
        unsigned int nFrames,
        double streamTime,
        RtAudioStreamStatus status,
        void* userData);

    static int wasapiOutputCallback(
        void* outputBuffer,
        void* inputBuffer,
        unsigned int nFrames,
        double streamTime,
        RtAudioStreamStatus status,
        void* userData);

    std::size_t calculatePrebufferSamples() const;
    void waitForPrebuffer();

    Config config_;
    BridgeState state_;

    std::unique_ptr<RtAudio> asioInput_;
    std::unique_ptr<RtAudio> wasapiOutput_;

    bool running_ = false;
};