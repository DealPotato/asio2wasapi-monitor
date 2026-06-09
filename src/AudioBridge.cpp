#include "AudioBridge.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

AudioBridge::AudioBridge(const Config& config)
    : config_(config), state_(config)
{
}

AudioBridge::~AudioBridge()
{
    try
    {
        stop();
    }
    catch (...)
    {
    }
}

void AudioBridge::start()
{
    if (running_)
        return;

    asioInput_ = std::make_unique<RtAudio>(RtAudio::WINDOWS_ASIO);
    wasapiOutput_ = std::make_unique<RtAudio>(RtAudio::WINDOWS_WASAPI);

    RtAudio::StreamParameters inputParams;
    inputParams.deviceId = config_.asioDeviceId;
    inputParams.nChannels = config_.inputChannel;
    inputParams.firstChannel = 0;

    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = config_.wasapiDeviceId;
    outputParams.nChannels = 2;
    outputParams.firstChannel = 0;

    RtAudio::StreamOptions inputOptions;
    inputOptions.flags = RTAUDIO_MINIMIZE_LATENCY;
    inputOptions.streamName = "ASIO2WASAPI ASIO Input";

    RtAudio::StreamOptions outputOptions;
    outputOptions.flags = RTAUDIO_MINIMIZE_LATENCY;
    outputOptions.streamName = "ASIO2WASAPI WASAPI Output";

    unsigned int inputBufferFrames = config_.bufferFrames;

    asioInput_->openStream(
        nullptr,
        &inputParams,
        RTAUDIO_FLOAT32,
        config_.sampleRate,
        &inputBufferFrames,
        &AudioBridge::asioInputCallback,
        &state_,
        &inputOptions
    );

    unsigned int outputBufferFrames = config_.bufferFrames;

    wasapiOutput_->openStream(
        &outputParams,
        nullptr,
        RTAUDIO_FLOAT32,
        config_.sampleRate,
        &outputBufferFrames,
        &AudioBridge::wasapiOutputCallback,
        &state_,
        &outputOptions
    );

    asioInput_->startStream();

    waitForPrebuffer();

    wasapiOutput_->startStream();

    running_ = true;
}

void AudioBridge::stop()
{
    if (wasapiOutput_)
    {
        if (wasapiOutput_->isStreamRunning())
            wasapiOutput_->stopStream();

        if (wasapiOutput_->isStreamOpen())
            wasapiOutput_->closeStream();
    }

    if (asioInput_)
    {
        if (asioInput_->isStreamRunning())
            asioInput_->stopStream();

        if (asioInput_->isStreamOpen())
            asioInput_->closeStream();
    }

    wasapiOutput_.reset();
    asioInput_.reset();

    running_ = false;
}

bool AudioBridge::isRunning() const
{
    return running_;
}

float AudioBridge::inputPeak() const
{
    return state_.inputPeak.load(std::memory_order_relaxed);
}

std::size_t AudioBridge::ringAvailable() const
{
    return state_.ring.available();
}

unsigned long long AudioBridge::inputCallbacks() const
{
    return state_.inputCallbacks.load(std::memory_order_relaxed);
}

unsigned long long AudioBridge::outputCallbacks() const
{
    return state_.outputCallbacks.load(std::memory_order_relaxed);
}

unsigned long long AudioBridge::underruns() const
{
    return state_.underruns.load(std::memory_order_relaxed);
}

unsigned long long AudioBridge::overruns() const
{
    return state_.overruns.load(std::memory_order_relaxed);
}

int AudioBridge::asioInputCallback(
    void* outputBuffer,
    void* inputBuffer,
    unsigned int nFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData)
{
    (void)outputBuffer;
    (void)streamTime;

    auto* state = static_cast<BridgeState*>(userData);

    if (status)
        state->underruns.fetch_add(1, std::memory_order_relaxed);

    if (!inputBuffer)
        return 0;

    const auto* input = static_cast<const float*>(inputBuffer);

    const unsigned int requestedChannels = state->config.inputChannel;
    const unsigned int selectedIndex = state->config.inputChannel - 1;

    float peak = 0.0f;

    for (unsigned int i = 0; i < nFrames; ++i)
    {
        const float sample = input[i * requestedChannels + selectedIndex];

        if (!state->ring.writeOne(sample))
            state->overruns.fetch_add(1, std::memory_order_relaxed);

        peak = std::max(peak, std::fabs(sample));
    }

    state->inputPeak.store(peak, std::memory_order_relaxed);
    state->inputCallbacks.fetch_add(1, std::memory_order_relaxed);

    return 0;
}

int AudioBridge::wasapiOutputCallback(
    void* outputBuffer,
    void* inputBuffer,
    unsigned int nFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData)
{
    (void)inputBuffer;
    (void)streamTime;

    auto* state = static_cast<BridgeState*>(userData);

    if (status)
        state->underruns.fetch_add(1, std::memory_order_relaxed);

    auto* output = static_cast<float*>(outputBuffer);

    for (unsigned int i = 0; i < nFrames; ++i)
    {
        float sample = 0.0f;

        if (!state->ring.readOne(sample))
            state->underruns.fetch_add(1, std::memory_order_relaxed);

        sample *= state->config.gain;

        output[i * 2 + 0] = sample;
        output[i * 2 + 1] = sample;
    }

    state->outputCallbacks.fetch_add(1, std::memory_order_relaxed);

    return 0;
}

std::size_t AudioBridge::calculatePrebufferSamples() const
{
    if (config_.prebufferMs == 0)
        return 0;

    const auto samplesFromMs =
        static_cast<std::size_t>(
            (static_cast<unsigned long long>(config_.sampleRate) * config_.prebufferMs) / 1000
        );

    return std::max<std::size_t>(samplesFromMs, config_.bufferFrames);
}

void AudioBridge::waitForPrebuffer()
{
    const std::size_t targetSamples = calculatePrebufferSamples();

    if (targetSamples == 0)
        return;

    std::cout << "Waiting for prebuffer: " << targetSamples << " samples...\n";

    const auto start = std::chrono::steady_clock::now();

    while (state_.ring.available() < targetSamples)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        const auto elapsed = std::chrono::steady_clock::now() - start;

        if (elapsed > std::chrono::milliseconds(1000))
        {
            std::cout << "Warning: prebuffer timeout. Starting output anyway.\n";
            break;
        }
    }

    std::cout << "Prebuffer ready: " << state_.ring.available() << " samples.\n";
}