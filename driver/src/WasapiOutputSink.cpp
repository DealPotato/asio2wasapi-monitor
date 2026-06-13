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
    unsigned int bufferFrames,
    float outputGain,
    bool useDefaultDevice,
    const std::string& preferredDeviceName,
    bool exclusiveMode)
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
    outputGain_.store(outputGain);
    useDefaultDevice_ = useDefaultDevice;
    preferredDeviceName_ = preferredDeviceName;
    exclusiveMode_ = exclusiveMode;

    try
    {
        const unsigned int deviceId = findOutputDevice();

        if (deviceId == 0)
        {
            lastError_ = "No WASAPI output device found.";
            debugLog("[ASIO2WASAPI] WASAPI start failed: no output device\n");
            return false;
        }

        const auto info = audio_->getDeviceInfo(deviceId);

        RtAudio::StreamParameters outputParameters;
        outputParameters.deviceId = deviceId;
        outputParameters.nChannels = 2;
        outputParameters.firstChannel = 0;

        RtAudio::StreamOptions options;
        options.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME;
        options.streamName = "ASIO2WASAPI WASAPI Output";

        if (exclusiveMode_)
        {
            options.flags |= RTAUDIO_HOG_DEVICE;
        }

        try
        {
            audio_->openStream(
                &outputParameters,
                nullptr,
                RTAUDIO_FLOAT32,
                sampleRate,
                &bufferFrames,
                &WasapiOutputSink::audioCallback,
                this,
                &options);
        }
        catch (const std::exception& e)
        {
            if (!exclusiveMode_)
            {
                throw;
            }

            char message[512] = {};
            std::snprintf(
                message,
                sizeof(message),
                "[ASIO2WASAPI] WASAPI exclusive mode failed, falling back to shared mode: %s\n",
                e.what());

            debugLog(message);

            options.flags &= ~RTAUDIO_HOG_DEVICE;
            exclusiveMode_ = false;

            audio_->openStream(
                &outputParameters,
                nullptr,
                RTAUDIO_FLOAT32,
                sampleRate,
                &bufferFrames,
                &WasapiOutputSink::audioCallback,
                this,
                &options);
        }

        audio_->startStream();

        running_ = true;

        char message[512] = {};
        std::snprintf(
            message,
            sizeof(message),
            "[ASIO2WASAPI] WASAPI output started: device='%s' bufferFrames=%u exclusive=%s\n",
            info.name.c_str(),
            bufferFrames,
            exclusiveMode_ ? "true" : "false");

        debugLog(message);

        return true;
    }
    catch (const std::exception& e)
    {
        lastError_ = e.what();

        char message[512] = {};
        std::snprintf(
            message,
            sizeof(message),
            "[ASIO2WASAPI] WASAPI output start exception: %s\n",
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

unsigned int WasapiOutputSink::findOutputDevice() const
{
    if (!audio_)
        return 0;

    const unsigned int defaultDeviceId = audio_->getDefaultOutputDevice();

    if (useDefaultDevice_ || preferredDeviceName_.empty())
    {
        return defaultDeviceId;
    }

    for (const auto deviceId : audio_->getDeviceIds())
    {
        try
        {
            const auto info = audio_->getDeviceInfo(deviceId);

            if (info.outputChannels == 0)
                continue;

            char message[512] = {};
            std::snprintf(
                message,
                sizeof(message),
                "[ASIO2WASAPI] WASAPI output candidate: id=%u name='%s' outputs=%u\n",
                deviceId,
                info.name.c_str(),
                info.outputChannels);

            debugLog(message);

            if (info.name.find(preferredDeviceName_) != std::string::npos)
            {
                std::snprintf(
                    message,
                    sizeof(message),
                    "[ASIO2WASAPI] WASAPI output selected: id=%u name='%s'\n",
                    deviceId,
                    info.name.c_str());

                debugLog(message);

                return deviceId;
            }
        }
        catch (const std::exception& e)
        {
            char message[512] = {};
            std::snprintf(
                message,
                sizeof(message),
                "[ASIO2WASAPI] WASAPI output probe failed: id=%u error='%s'\n",
                deviceId,
                e.what());

            debugLog(message);
        }
    }

    debugLog("[ASIO2WASAPI] Preferred WASAPI output was not found, falling back to default output\n");

    return defaultDeviceId;
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

    const float outputGain = outputGain_.load();

    for (unsigned int i = 0; i < nBufferFrames * 2; ++i)
    {
        output[i] = std::clamp(output[i] * outputGain, -1.0f, 1.0f);
    }

    return 0;
}