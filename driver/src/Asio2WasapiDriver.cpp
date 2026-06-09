#include "Asio2WasapiDriver.h"

#include <algorithm>
#include <chrono>
#include <cstring>

const CLSID CLSID_Asio2WasapiVirtualAsio =
{
    0x85b9bdb2,
    0x2f44,
    0x4d13,
    { 0x9c, 0x7a, 0x2f, 0x28, 0x63, 0xa0, 0xd1, 0xd0 }
};

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
    sysHandle_ = sysHandle;
    copyString(errorMessage_, "No error", sizeof(errorMessage_));
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
    if (running_)
        return ASE_OK;

    running_ = true;
    activeBufferIndex_ = 0;

    callbackThread_ = std::thread(&Asio2WasapiDriver::callbackLoop, this);

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::stop()
{
    if (!running_)
        return ASE_OK;

    running_ = false;

    if (callbackThread_.joinable())
        callbackThread_.join();

    return ASE_OK;
}

ASIOError Asio2WasapiDriver::getChannels(long* numInputChannels, long* numOutputChannels)
{
    if (!numInputChannels || !numOutputChannels)
        return ASE_InvalidParameter;

    *numInputChannels = 2;
    *numOutputChannels = 2;

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
    if (!minSize || !maxSize || !preferredSize || !granularity)
        return ASE_InvalidParameter;

    *minSize = 64;
    *maxSize = 512;
    *preferredSize = 128;
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

ASIOError Asio2WasapiDriver::controlPanel()
{
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

void Asio2WasapiDriver::callbackLoop()
{
    const auto bufferDuration =
        std::chrono::duration<double>(
            static_cast<double>(bufferSize_) / static_cast<double>(sampleRate_));

    auto nextWake = std::chrono::steady_clock::now();

    while (running_)
    {
        nextWake += std::chrono::duration_cast<std::chrono::steady_clock::duration>(bufferDuration);

        if (callbacks_)
        {
            clearBuffers(activeBufferIndex_);

            callbacks_->bufferSwitch(activeBufferIndex_, ASIOFalse);

            clearBuffers(activeBufferIndex_);

            samplePosition_.fetch_add(
                static_cast<unsigned long long>(bufferSize_),
                std::memory_order_relaxed);

            activeBufferIndex_ = 1 - activeBufferIndex_;
        }

        std::this_thread::sleep_until(nextWake);
    }
}

void Asio2WasapiDriver::clearBuffers(long activeBuffer)
{
    if (activeBuffer < 0 || activeBuffer > 1)
        return;

    for (auto& info : bufferInfos_)
    {
        if (!info.buffers[activeBuffer])
            continue;

        auto* buffer = static_cast<float*>(info.buffers[activeBuffer]);

        std::fill(
            buffer,
            buffer + bufferSize_,
            0.0f);
    }
}