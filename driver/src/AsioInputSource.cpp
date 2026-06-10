#include "AsioInputSource.h"
#include "DebugLog.h"

#include <algorithm>
#include <cstdio>
#include <exception>

AsioInputSource::AsioInputSource()
{
    try
    {
        audio_ = std::make_unique<RtAudio>(RtAudio::WINDOWS_ASIO);
    }
    catch (const std::exception& e)
    {
        lastError_ = e.what();
        debugLog("[ASIO2WASAPI] ASIO input init failed\n");
    }
}

AsioInputSource::~AsioInputSource()
{
    debugLog("[ASIO2WASAPI] AsioInputSource destructor called\n");
    stop();
}

bool AsioInputSource::start(
    MonoRingBuffer* ringBuffer,
    unsigned int sampleRate,
    unsigned int bufferFrames,
    unsigned int sourceChannel,
    const std::string& preferredDeviceName)
{
    debugLog("[ASIO2WASAPI] ASIO input start requested\n");

    if (!audio_)
    {
        debugLog("[ASIO2WASAPI] ASIO input start failed: audio_ is null\n");
        return false;
    }

    if (!ringBuffer)
    {
        debugLog("[ASIO2WASAPI] ASIO input start failed: ringBuffer is null\n");
        return false;
    }

    if (running_)
    {
        debugLog("[ASIO2WASAPI] ASIO input start ignored: already running\n");
        return true;
    }

    ringBuffer_ = ringBuffer;
    sourceChannel_ = sourceChannel;
    preferredDeviceName_ = preferredDeviceName;
    try
    {
        const unsigned int deviceId = findInputDevice();

        if (deviceId == 0)
        {
            lastError_ = "No ASIO input device found.";
            debugLog("[ASIO2WASAPI] ASIO input start failed: no input device\n");
            return false;
        }
        
        const auto info = audio_->getDeviceInfo(deviceId);

        openedChannels_ = std::min<unsigned int>(
            info.inputChannels,
            std::max<unsigned int>(2, sourceChannel_ + 1));

        if (sourceChannel_ >= openedChannels_)
        {
            lastError_ = "Requested source channel is not available.";
            debugLog("[ASIO2WASAPI] ASIO input start failed: source channel unavailable\n");
            return false;
        }

        RtAudio::StreamParameters inputParameters;
        inputParameters.deviceId = deviceId;
        inputParameters.nChannels = openedChannels_;
        inputParameters.firstChannel = 0;

        RtAudio::StreamOptions options;
        options.flags = RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_SCHEDULE_REALTIME;
        options.streamName = "ASIO2WASAPI Hardware ASIO Input";

        audio_->openStream(
            nullptr,
            &inputParameters,
            RTAUDIO_FLOAT32,
            sampleRate,
            &bufferFrames,
            &AsioInputSource::audioCallback,
            this,
            &options);

        audio_->startStream();

        running_ = true;

        char message[512] = {};
        std::snprintf(
            message,
            sizeof(message),
            "[ASIO2WASAPI] ASIO input started: device='%s' openedChannels=%u sourceChannel=%u\n",
            info.name.c_str(),
            openedChannels_,
            sourceChannel_);

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
            "[ASIO2WASAPI] ASIO input start exception: %s\n",
            lastError_.c_str());

        debugLog(message);

        return false;
    }
}

void AsioInputSource::stop()
{
    if (!audio_ || !running_)
        return;

    debugLog("[ASIO2WASAPI] ASIO input stop requested\n");

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
        debugLog("[ASIO2WASAPI] ASIO input stop exception\n");
    }

    running_ = false;
    ringBuffer_ = nullptr;

    debugLog("[ASIO2WASAPI] ASIO input stopped\n");
}

bool AsioInputSource::isRunning() const
{
    return running_;
}

const std::string& AsioInputSource::lastError() const
{
    return lastError_;
}

int AsioInputSource::audioCallback(
    void* outputBuffer,
    void* inputBuffer,
    unsigned int nBufferFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData)
{
    (void)outputBuffer;
    (void)streamTime;

    auto* source = static_cast<AsioInputSource*>(userData);

    if (!source)
        return 0;

    return source->capture(inputBuffer, nBufferFrames, status);
}

int AsioInputSource::capture(
    void* inputBuffer,
    unsigned int nBufferFrames,
    RtAudioStreamStatus status)
{
    (void)status;

    if (!inputBuffer || !ringBuffer_)
        return 0;

    const auto* input = static_cast<const float*>(inputBuffer);

    captureScratch_.resize(nBufferFrames);

    for (unsigned int i = 0; i < nBufferFrames; ++i)
    {
        captureScratch_[i] =
            input[(i * openedChannels_) + sourceChannel_];
    }

    ringBuffer_->write(captureScratch_.data(), nBufferFrames);

    return 0;
}

unsigned int AsioInputSource::findInputDevice() const
{
    if (!audio_)
        return 0;

    unsigned int fallbackDevice = 0;

    for (const auto deviceId : audio_->getDeviceIds())
    {
        try
        {
            const auto info = audio_->getDeviceInfo(deviceId);

            if (info.inputChannels == 0)
                continue;

            if (info.name.find("ASIO2WASAPI") != std::string::npos)
            {
                debugLog("[ASIO2WASAPI] ASIO input candidate skipped: self driver\n");
                continue;
            }

            char message[512] = {};
            std::snprintf(
                message,
                sizeof(message),
                "[ASIO2WASAPI] ASIO input candidate: id=%u name='%s' inputs=%u\n",
                deviceId,
                info.name.c_str(),
                info.inputChannels);

            debugLog(message);

            if (fallbackDevice == 0)
                fallbackDevice = deviceId;

            if (!preferredDeviceName_.empty() &&
                info.name.find(preferredDeviceName_) != std::string::npos)
            {
                return deviceId;
            }

            if (info.name.find("Focusrite") != std::string::npos ||
                info.name.find("Scarlett") != std::string::npos)
            {
                return deviceId;
            }
        }
        catch (const std::exception& e)
        {
            char message[512] = {};
            std::snprintf(
                message,
                sizeof(message),
                "[ASIO2WASAPI] ASIO input device probe failed: id=%u error='%s'\n",
                deviceId,
                e.what());

            debugLog(message);
        }
    }

    return fallbackDevice;
}