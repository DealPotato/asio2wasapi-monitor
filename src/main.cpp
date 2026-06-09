#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <RtAudio.h>

static bool containsApi(const std::vector<RtAudio::Api>& apis, RtAudio::Api api)
{
    return std::find(apis.begin(), apis.end(), api) != apis.end();
}

static void printSampleRates(const std::vector<unsigned int>& rates)
{
    if (rates.empty())
    {
        std::cout << "none reported";
        return;
    }

    for (size_t i = 0; i < rates.size(); ++i)
    {
        std::cout << rates[i];
        if (i + 1 < rates.size())
            std::cout << ", ";
    }
}

static void printDevicesForApi(RtAudio::Api api)
{
    std::cout << "\n============================================================\n";
    std::cout << RtAudio::getApiDisplayName(api)
              << " (" << RtAudio::getApiName(api) << ")\n";
    std::cout << "============================================================\n";

    RtAudio audio(api);

    const auto deviceIds = audio.getDeviceIds();

    if (deviceIds.empty())
    {
        std::cout << "No devices found for this API.\n";
        return;
    }

    for (const auto deviceId : deviceIds)
    {
        const auto info = audio.getDeviceInfo(deviceId);

        std::cout << "\n[" << deviceId << "] " << info.name << "\n";
        std::cout << "    Inputs:  " << info.inputChannels << "\n";
        std::cout << "    Outputs: " << info.outputChannels << "\n";
        std::cout << "    Duplex:  " << info.duplexChannels << "\n";

        std::cout << "    Default input:  "
                  << (info.isDefaultInput ? "yes" : "no") << "\n";

        std::cout << "    Default output: "
                  << (info.isDefaultOutput ? "yes" : "no") << "\n";

        std::cout << "    Current sample rate:   "
                  << info.currentSampleRate << "\n";

        std::cout << "    Preferred sample rate: "
                  << info.preferredSampleRate << "\n";

        std::cout << "    Supported sample rates: ";
        printSampleRates(info.sampleRates);
        std::cout << "\n";
    }
}

int main()
{
    std::cout << "ASIO2WASAPI Monitor - Device Probe\n";

    std::vector<RtAudio::Api> compiledApis;
    RtAudio::getCompiledApi(compiledApis);

    std::cout << "\nCompiled APIs:\n";
    for (const auto api : compiledApis)
    {
        std::cout << "  - " << RtAudio::getApiDisplayName(api)
                  << " (" << RtAudio::getApiName(api) << ")\n";
    }

    const bool hasAsio = containsApi(compiledApis, RtAudio::WINDOWS_ASIO);
    const bool hasWasapi = containsApi(compiledApis, RtAudio::WINDOWS_WASAPI);

    std::cout << "\nRequired backends:\n";
    std::cout << "  ASIO:   " << (hasAsio ? "available" : "missing") << "\n";
    std::cout << "  WASAPI: " << (hasWasapi ? "available" : "missing") << "\n";

    if (hasAsio)
        printDevicesForApi(RtAudio::WINDOWS_ASIO);

    if (hasWasapi)
        printDevicesForApi(RtAudio::WINDOWS_WASAPI);

    std::cout << "\nProbe complete.\n";
    return 0;
}