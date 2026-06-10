#include "Asio2WasapiDriver.h"
#include "DebugLog.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <thread>
#include <vector>

static void copyString(char* destination, const char* source, std::size_t maxLength)
{
    if (!destination)
        return;

    std::strncpy(destination, source, maxLength - 1);
    destination[maxLength - 1] = '\0';
}

static void setAsioSamples(ASIOSamples* samples, unsigned long long value)
{
    if (!samples)
        return;

    samples->hi = static_cast<unsigned long>(value >> 32);
    samples->lo = static_cast<unsigned long>(value & 0xffffffffULL);
}

static void setAsioTimeStamp(ASIOTimeStamp* timestamp, unsigned long long value)
{
    if (!timestamp)
        return;

    timestamp->hi = static_cast<unsigned long>(value >> 32);
    timestamp->lo = static_cast<unsigned long>(value & 0xffffffffULL);
}

Asio2WasapiDriver::Asio2WasapiDriver() = default;

Asio2WasapiDriver::~Asio2WasapiDriver()
{
    stop();
    disposeBuffers();
}

HRESULT STDMETHODCALLTYPE Asio2WasapiDriver::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObject = static_cast<IASIO*>(this);
        AddRef();
        return S_OK;
    }

    // Some ASIO hosts request the ASIO interface through a SDK-specific IID.
    // For this skeleton we are permissive and return IASIO for non-IUnknown requests too.
    *ppvObject = static_cast<IASIO*>(this);
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE Asio2WasapiDriver::AddRef()
{
    return ++refCount_;
}

ULONG STDMETHODCALLTYPE Asio2WasapiDriver::Release()
{
    const ULONG count = --refCount_;

    if (count == 0)
        delete this;

    return count;
}

ASIOBool Asio2WasapiDriver::init(void* sysHandle)
{
    debugLog("[ASIO2WASAPI] init called\n");
    return ASIOTrue;
}

void Asio2WasapiDriver::getDriverName(char* name)
{
    copyString(name, "ASIO2WASAPI Virtual ASIO", 64);
}

long Asio2WasapiDriver::getDriverVersion()
{
    return 1;
}

void Asio2WasapiDriver::getErrorMessage(char* string)
{
    copyString(string, errorMessage_, 128);
}

ASIOError Asio2WasapiDriver::start()
{
    debugLog("[ASIO2WASAPI] start called\n");

    if (!callbacks_)
    {
        debugLog("[ASIO2WASAPI] start failed: callbacks_ is null\n");
        return ASE_NotPresent;
    }

    bool expected = false;

    if (!running_.compare_exchange_strong(expected, true))
    {
        debugLog("[ASIO2WASAPI] start ignored: already running\n");
        return ASE_OK;
    }

    inputRing_.clear();
    outputRing_.clear();

    const bool asioInputStarted = asioInput_.start(
        &inputRing_,
        static_cast<unsigned int>(sampleRate_),
        static_cast<unsigned int>(bufferSize_),
        1); // Scarlett input 2, zero-based.

    if (!asioInputStarted)
    {
        debugLog("[ASIO2WASAPI] warning: ASIO hardware input did not start\n");
    }

    const unsigned int wasapiBufferFrames = 256;

    const bool wasapiStarted = wasapiOutput_.start(
        &outputRing_,
        static_cast<unsigned int>(sampleRate_),
        wasapiBufferFrames);

    if (!wasapiStarted)
    {
        debugLog("[ASIO2WASAPI] warning: WASAPI output did not start\n");
    }

    callbackThread_ = std::thread(&Asio2WasapiDriver::callbackLoop, this);

    debugLog("[ASIO2WASAPI] callback thread started\n");

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::stop()
{
    debugLog("[ASIO2WASAPI] stop called\n");

    if (!running_.exchange(false))
    {
        debugLog("[ASIO2WASAPI] stop ignored: not running\n");
        return ASE_OK;
    }

    if (callbackThread_.joinable())
    {
        callbackThread_.join();
        debugLog("[ASIO2WASAPI] callback thread joined\n");
    }

    asioInput_.stop();
    inputRing_.clear();

    wasapiOutput_.stop();
    outputRing_.clear();

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::getChannels(
    long* numInputChannels,
    long* numOutputChannels)
{
    debugLog("[ASIO2WASAPI] getChannels called\n");

    if (numInputChannels)
        *numInputChannels = 2;

    if (numOutputChannels)
        *numOutputChannels = 2;

    debugLog("[ASIO2WASAPI] getChannels returning 2 in / 2 out\n");

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::getLatencies(long* inputLatency, long* outputLatency)
{
    if (!inputLatency || !outputLatency)
        return ASE_InvalidParameter;

    *inputLatency = bufferSize_;
    *outputLatency = bufferSize_;

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::getBufferSize(
    long* minSize,
    long* maxSize,
    long* preferredSize,
    long* granularity)
{
    OutputDebugStringA("[ASIO2WASAPI] getBufferSize called\n");

    if (minSize)
        *minSize = 64;

    if (maxSize)
        *maxSize = 512;

    if (preferredSize)
        *preferredSize = 128;

    if (granularity)
        *granularity = -1;

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::canSampleRate(ASIOSampleRate sampleRate)
{
    if (sampleRate == 44100.0 || sampleRate == 48000.0)
        return ASE_OK;

    return ASE_NoClock;
}

ASIOError Asio2WasapiDriver::getSampleRate(ASIOSampleRate* sampleRate)
{
    if (!sampleRate)
        return ASE_InvalidParameter;

    *sampleRate = sampleRate_;
    return ASE_OK;
}

ASIOError Asio2WasapiDriver::setSampleRate(ASIOSampleRate sampleRate)
{
    if (canSampleRate(sampleRate) != ASE_OK)
        return ASE_NoClock;

    sampleRate_ = sampleRate;
    return ASE_OK;
}

ASIOError Asio2WasapiDriver::getClockSources(ASIOClockSource* clocks, long* numSources)
{
    if (!numSources)
        return ASE_InvalidParameter;

    if (!clocks)
    {
        *numSources = 1;
        return ASE_OK;
    }

    if (*numSources < 1)
        return ASE_InvalidParameter;

    clocks[0].index = 0;
    clocks[0].associatedChannel = -1;
    clocks[0].associatedGroup = -1;
    clocks[0].isCurrentSource = ASIOTrue;
    copyString(clocks[0].name, "Internal", sizeof(clocks[0].name));

    *numSources = 1;

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::setClockSource(long reference)
{
    if (reference != 0)
        return ASE_NotPresent;

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::getSamplePosition(ASIOSamples* sPos, ASIOTimeStamp* tStamp)
{
    if (!sPos || !tStamp)
        return ASE_InvalidParameter;

    const auto pos = samplePosition_.load(std::memory_order_relaxed);

    setAsioSamples(sPos, pos);
    setAsioTimeStamp(tStamp, 0);

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::getChannelInfo(ASIOChannelInfo* info)
{
    debugLog("[ASIO2WASAPI] getChannelInfo called\n");

    if (!info)
        return ASE_InvalidParameter;

    if (info->channel < 0 || info->channel >= 2)
        return ASE_InvalidParameter;

    info->isActive = ASIOTrue;
    info->channelGroup = 0;
    info->type = ASIOSTFloat32LSB;

    if (info->isInput)
    {
        if (info->channel == 0)
            copyString(info->name, "Input 1", sizeof(info->name));
        else
            copyString(info->name, "Input 2", sizeof(info->name));
    }
    else
    {
        if (info->channel == 0)
            copyString(info->name, "Output 1", sizeof(info->name));
        else
            copyString(info->name, "Output 2", sizeof(info->name));
    }

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::createBuffers(
    ASIOBufferInfo* bufferInfos,
    long numChannels,
    long bufferSize,
    ASIOCallbacks* callbacks)
{   
    debugLog("[ASIO2WASAPI] createBuffers called\n");

    if (!bufferInfos || !callbacks || numChannels <= 0 || bufferSize <= 0)
        return ASE_InvalidParameter;

    bufferSize_ = bufferSize;
    callbacks_ = callbacks;

    bufferInfos_.assign(bufferInfos, bufferInfos + numChannels);
    ownedBuffers_.clear();
    ownedBuffers_.resize(static_cast<std::size_t>(numChannels));

    for (long i = 0; i < numChannels; ++i)
    {
        ownedBuffers_[i][0].assign(static_cast<std::size_t>(bufferSize_), 0.0f);
        ownedBuffers_[i][1].assign(static_cast<std::size_t>(bufferSize_), 0.0f);

        bufferInfos_[i].buffers[0] = ownedBuffers_[i][0].data();
        bufferInfos_[i].buffers[1] = ownedBuffers_[i][1].data();

        bufferInfos[i].buffers[0] = bufferInfos_[i].buffers[0];
        bufferInfos[i].buffers[1] = bufferInfos_[i].buffers[1];
    }

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::disposeBuffers()
{
    stop();

    callbacks_ = nullptr;
    bufferInfos_.clear();
    ownedBuffers_.clear();

    return ASE_OK;
}

void Asio2WasapiDriver::generateTestInputTone(long activeBuffer)
{
    if (activeBuffer < 0 || activeBuffer > 1)
        return;

    constexpr double frequency = 440.0;
    constexpr double amplitude = 0.15;
    constexpr double twoPi = 6.28318530717958647692;

    const double phaseIncrement =
        twoPi * frequency / static_cast<double>(sampleRate_);

    std::vector<float> tone(static_cast<std::size_t>(bufferSize_));

    for (long i = 0; i < bufferSize_; ++i)
    {
        tone[static_cast<std::size_t>(i)] =
            static_cast<float>(std::sin(testTonePhase_) * amplitude);

        testTonePhase_ += phaseIncrement;

        if (testTonePhase_ >= twoPi)
            testTonePhase_ -= twoPi;
    }

    for (auto& info : bufferInfos_)
    {
        if (!info.isInput)
            continue;

        if (!info.buffers[activeBuffer])
            continue;

        auto* buffer = static_cast<float*>(info.buffers[activeBuffer]);

        if (info.channelNum == 0)
        {
            std::copy(
                tone.begin(),
                tone.end(),
                buffer);
        }
        else
        {
            std::fill(
                buffer,
                buffer + bufferSize_,
                0.0f);
        }
    }
}

void Asio2WasapiDriver::fillHardwareInputFromRing(long activeBuffer)
{
    if (activeBuffer < 0 || activeBuffer > 1)
        return;

    inputScratch_.resize(static_cast<std::size_t>(bufferSize_));

    inputRing_.read(
        inputScratch_.data(),
        static_cast<std::size_t>(bufferSize_));

    for (auto& info : bufferInfos_)
    {
        if (!info.isInput)
            continue;

        if (!info.buffers[activeBuffer])
            continue;

        auto* buffer = static_cast<float*>(info.buffers[activeBuffer]);

        if (info.channelNum == 0)
        {
            std::copy(
                inputScratch_.begin(),
                inputScratch_.end(),
                buffer);
        }
        else
        {
            std::fill(
                buffer,
                buffer + bufferSize_,
                0.0f);
        }
    }
}

ASIOError Asio2WasapiDriver::controlPanel()
{
    debugLog("[ASIO2WASAPI] controlPanel called\n");

    MessageBoxA(
        nullptr,
        "ASIO2WASAPI Virtual ASIO skeleton driver.\n\nNo control panel is implemented yet.",
        "ASIO2WASAPI Virtual ASIO",
        MB_OK | MB_ICONINFORMATION);

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::future(long selector, void* opt)
{
    (void)selector;
    (void)opt;

    return ASE_NotPresent;
}

ASIOError Asio2WasapiDriver::outputReady()
{
    return ASE_OK;
}


void Asio2WasapiDriver::writeOutputToRing(long activeBuffer)
{
    if (activeBuffer < 0 || activeBuffer > 1)
        return;

    const float* left = nullptr;
    const float* right = nullptr;

    for (const auto& info : bufferInfos_)
    {
        if (info.isInput)
            continue;

        if (!info.buffers[activeBuffer])
            continue;

        if (info.channelNum == 0)
            left = static_cast<const float*>(info.buffers[activeBuffer]);

        if (info.channelNum == 1)
            right = static_cast<const float*>(info.buffers[activeBuffer]);
    }

    if (!left || !right)
        return;

    outputRing_.writeFromPlanar(
        left,
        right,
        static_cast<std::size_t>(bufferSize_));
}

void Asio2WasapiDriver::callbackLoop()
{
    debugLog("[ASIO2WASAPI] callbackLoop entered\n");

    auto nextWake = std::chrono::steady_clock::now();

    while (running_.load(std::memory_order_acquire))
    {
        if (callbacks_)
        {
            if (enableTestInputTone_)
                generateTestInputTone(activeBufferIndex_);
            else
                fillHardwareInputFromRing(activeBufferIndex_);

            // Host yazmadan önce output buffer'ı temizle.
            clearOutputBuffers(activeBufferIndex_);

            // REAPER burada bizim output buffer'a ses yazmalı.
            callbacks_->bufferSwitch(activeBufferIndex_, ASIOFalse);

            const float peak = measureOutputPeak(activeBufferIndex_);
            outputPeak_.store(peak, std::memory_order_relaxed);
            writeOutputToRing(activeBufferIndex_);

            const auto count =
                callbackCount_.fetch_add(1, std::memory_order_relaxed) + 1;

            debugPrintOutputPeak(peak, count);

            samplePosition_.fetch_add(
                static_cast<unsigned long long>(bufferSize_),
                std::memory_order_relaxed);

            activeBufferIndex_ = 1 - activeBufferIndex_;
        }

        const double bufferMs =
            (static_cast<double>(bufferSize_) / sampleRate_) * 1000.0;

        const auto bufferDuration = std::chrono::duration<double>(
            static_cast<double>(bufferSize_) / static_cast<double>(sampleRate_));

        nextWake += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            bufferDuration);

        std::this_thread::sleep_until(nextWake);
    }

    debugLog("[ASIO2WASAPI] callbackLoop exited\n");
}

void Asio2WasapiDriver::clearInputBuffers(long activeBuffer)
{
    if (activeBuffer < 0 || activeBuffer > 1)
        return;

    for (auto& info : bufferInfos_)
    {
        if (!info.isInput)
            continue;

        if (!info.buffers[activeBuffer])
            continue;

        auto* buffer = static_cast<float*>(info.buffers[activeBuffer]);

        std::fill(
            buffer,
            buffer + bufferSize_,
            0.0f);
    }
}

void Asio2WasapiDriver::clearOutputBuffers(long activeBuffer)
{
    if (activeBuffer < 0 || activeBuffer > 1)
        return;

    for (auto& info : bufferInfos_)
    {
        if (info.isInput)
            continue;

        if (!info.buffers[activeBuffer])
            continue;

        auto* buffer = static_cast<float*>(info.buffers[activeBuffer]);

        std::fill(
            buffer,
            buffer + bufferSize_,
            0.0f);
    }
}

float Asio2WasapiDriver::measureOutputPeak(long activeBuffer) const
{
    if (activeBuffer < 0 || activeBuffer > 1)
        return 0.0f;

    float peak = 0.0f;

    for (const auto& info : bufferInfos_)
    {
        if (info.isInput)
            continue;

        if (!info.buffers[activeBuffer])
            continue;

        const auto* buffer = static_cast<const float*>(info.buffers[activeBuffer]);

        for (long i = 0; i < bufferSize_; ++i)
        {
            peak = std::max(peak, std::fabs(buffer[i]));
        }
    }

    return peak;
}

void Asio2WasapiDriver::debugPrintOutputPeak(float peak, unsigned long long callbackCount)
{
    if ((callbackCount % 200) != 0)
        return;

    char message[160] = {};

    std::snprintf(
        message,
        sizeof(message),
        "[ASIO2WASAPI] callback=%llu outputPeak=%f inputRingFrames=%zu outputRingFrames=%zu\n",
        callbackCount,
        peak,
        inputRing_.availableFrames(),
        outputRing_.availableFrames());

    debugLog(message);
}