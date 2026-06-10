#pragma once

#include <windows.h>

#include <windows.h>
#include <objbase.h>
#include <unknwn.h>

#include <asiosys.h>
#include <asio.h>
#include <iasiodrv.h>

#include <atomic>
#include <array>
#include <thread>
#include <vector>

extern const CLSID CLSID_Asio2WasapiVirtualAsio;

class Asio2WasapiDriver final : public IASIO
{
public:
    Asio2WasapiDriver();
    ~Asio2WasapiDriver();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

    ASIOBool init(void* sysHandle) override;
    void getDriverName(char* name) override;
    long getDriverVersion() override;
    void getErrorMessage(char* string) override;

    ASIOError start() override;
    ASIOError stop() override;

    ASIOError getChannels(long* numInputChannels, long* numOutputChannels) override;
    ASIOError getLatencies(long* inputLatency, long* outputLatency) override;
    ASIOError getBufferSize(long* minSize, long* maxSize, long* preferredSize, long* granularity) override;

    ASIOError canSampleRate(ASIOSampleRate sampleRate) override;
    ASIOError getSampleRate(ASIOSampleRate* sampleRate) override;
    ASIOError setSampleRate(ASIOSampleRate sampleRate) override;

    ASIOError getClockSources(ASIOClockSource* clocks, long* numSources) override;
    ASIOError setClockSource(long reference) override;

    ASIOError getSamplePosition(ASIOSamples* sPos, ASIOTimeStamp* tStamp) override;
    ASIOError getChannelInfo(ASIOChannelInfo* info) override;

    ASIOError createBuffers(
        ASIOBufferInfo* bufferInfos,
        long numChannels,
        long bufferSize,
        ASIOCallbacks* callbacks) override;

    ASIOError disposeBuffers() override;
    ASIOError controlPanel() override;
    ASIOError future(long selector, void* opt) override;
    ASIOError outputReady() override;

private:
    void callbackLoop();
    void clearInputBuffers(long activeBuffer);
    void clearOutputBuffers(long activeBuffer);
    float measureOutputPeak(long activeBuffer) const;
    void debugPrintOutputPeak(float peak, unsigned long long callbackCount) const;

    std::atomic<ULONG> refCount_{1};

    void* sysHandle_ = nullptr;

    ASIOSampleRate sampleRate_ = 48000.0;
    long bufferSize_ = 128;

    ASIOCallbacks* callbacks_ = nullptr;
    std::vector<ASIOBufferInfo> bufferInfos_;
    std::vector<std::array<std::vector<float>, 2>> ownedBuffers_;

    std::atomic<bool> running_{false};
    std::thread callbackThread_;

    std::atomic<unsigned long long> samplePosition_{0};
    std::atomic<float> outputPeak_{0.0f};
    std::atomic<unsigned long long> callbackCount_{0};
    long activeBufferIndex_ = 0;

    char errorMessage_[128] = "No error";
};