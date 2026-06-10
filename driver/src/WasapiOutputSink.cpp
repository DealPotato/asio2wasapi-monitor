#include "WasapiOutputSink.h"
#include "DebugLog.h"

#include <algorithm>
#include <cstdio>
#include <exception>

WasapiOutputSink::WasapiOutputSink()
{
    try
    {
        audio_ = std::make_unique<RtAudio>(RtAudio::WINDOWS_WASAPI);
    }
    catch (const std::exception& e)
    {
        lastError_ = e.what();
        debugLog("[ASIO2WASAPI] WASAPI init failed\n");
    }
}

WasapiOutputSink::~WasapiOutputSink()
{
    stop();
}

bool WasapiOutputSink::start(
    StereoRingBuffer* ringBuffer,
    unsigned int sampleRate,
    unsigned int bufferFrames)
{
    debugLog("[ASIO2WASAPI] WASAPI output start requested\n");

    if (!audio_)
    {
        debugLog("[ASIO2WASAPI] WASAPI start failed: audio_ is null\n");
        return false;
    }

    if (!ringBuffer)
    {
        debugLog("[ASIO2WASAPI] WASAPI start failed: ringBuffer is null\n");
        return false;
    }

    if (running_)
    {
        debugLog("[ASIO2WASAPI] WASAPI start ignored: already running\n");
        return true;
    }

    ringBuffer_ = ringBuffer;

    try
    {
        const unsigned int deviceId = audio_->getDefaultOutputDevice();

        if (deviceId == 0)
        {
            lastError_ = "No default WASAPI output device.";
            debugLog("[ASIO2WASAPI] WASAPI start failed: no default output device\n");
            return false;
        }

        RtAudio::StreamParameters outputParameters;
        outputParameters.deviceId = deviceId;
        outputParameters.nChannels = 2;
        outputParameters.firstChannel = 0;

        RtAudio::StreamOptions options;
        options.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME;
        options.streamName = "ASIO2WASAPI Virtual ASIO Output";

        audio_->openStream(
            &outputParameters,
            nullptr,
            RTAUDIO_FLOAT32,
            sampleRate,
            &bufferFrames,
            &WasapiOutputSink::audioCallback,
            this,
            &options);

        audio_->startStream();

        running_ = true;

        debugLog("[ASIO2WASAPI] WASAPI output started\n");
        return true;
    }
    catch (const std::exception& e)
    {
        lastError_ = e.what();

        char message[256] = {};
        std::snprintf(
            message,
            sizeof(message),
            "[ASIO2WASAPI] WASAPI start exception: %s\n",
            lastError_.c_str());

        debugLog(message);

        return false;
    }
}

void WasapiOutputSink::stop()
{
    if (!audio_ || !running_)
        return;

    debugLog("[ASIO2WASAPI] WASAPI output stop requested\n");

    try
    {
        if (audio_->isStreamRunning())
            audio_->stopStream();

        if (audio_->isStreamOpen())
            audio_->closeStream();
    }
    catch (const std::exception& e)
    {
        lastError_ = e.what();
        debugLog("[ASIO2WASAPI] WASAPI stop exception\n");
    }

    running_ = false;
    ringBuffer_ = nullptr;

    debugLog("[ASIO2WASAPI] WASAPI output stopped\n");
}

bool WasapiOutputSink::isRunning() const
{
    return running_;
}

const std::string& WasapiOutputSink::lastError() const
{
    return lastError_;
}

int WasapiOutputSink::audioCallback(
    void* outputBuffer,
    void* inputBuffer,
    unsigned int nBufferFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData)
{
    (void)inputBuffer;
    (void)streamTime;

    auto* sink = static_cast<WasapiOutputSink*>(userData);

    if (!sink)
        return 0;

    return sink->render(outputBuffer, nBufferFrames, status);
}

int WasapiOutputSink::render(
    void* outputBuffer,
    unsigned int nBufferFrames,
    RtAudioStreamStatus status)
{
    (void)status;

    auto* output = static_cast<float*>(outputBuffer);

    if (!output)
        return 0;

    if (!ringBuffer_)
    {
        std::fill(output, output + (nBufferFrames * 2), 0.0f);
        return 0;
    }

    ringBuffer_->readInterleaved(output, nBufferFrames);

    for (unsigned int i = 0; i < nBufferFrames * 2; ++i)
    {
        output[i] = std::clamp(output[i], -1.0f, 1.0f);
    }

    return 0;
}