#include "DeviceList.h"

#include <RtAudio.h>

#include <algorithm>
#include <iostream>
#include <vector>

static void printSampleRates(const std::vector<unsigned int>& rates)
{
    if (rates.empty())
    {
        std::cout << "none reported";
        return;
    }

    for (std::size_t i = 0; i < rates.size(); ++i)
    {
        std::cout << rates[i];

        if (i + 1 < rates.size())
            std::cout << ", ";
    }
}

static void listDevicesForApi(RtAudio::Api api)
{
    std::cout << "\n============================================================\n";
    std::cout << RtAudio::getApiDisplayName(api)
              << " (" << RtAudio::getApiName(api) << ")\n";
    std::cout << "============================================================\n";

    try
    {
        RtAudio audio(api);
        const auto deviceIds = audio.getDeviceIds();

        if (deviceIds.empty())
        {
            std::cout << "No devices found.\n";
            return;
        }

        for (const auto deviceId : deviceIds)
        {
            try
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
            catch (const std::exception& e)
            {
                std::cout << "\n[" << deviceId << "] Could not query device info: "
                          << e.what() << "\n";
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "Could not initialize API: " << e.what() << "\n";
    }
}

void listDevices()
{
    std::vector<RtAudio::Api> compiledApis;
    RtAudio::getCompiledApi(compiledApis);

    std::cout << "ASIO2WASAPI Monitor - Device List\n\n";

    std::cout << "Compiled APIs:\n";
    for (const auto api : compiledApis)
    {
        std::cout << "  - " << RtAudio::getApiDisplayName(api)
                  << " (" << RtAudio::getApiName(api) << ")\n";
    }

    const bool hasAsio =
        std::find(compiledApis.begin(), compiledApis.end(), RtAudio::WINDOWS_ASIO)
        != compiledApis.end();

    const bool hasWasapi =
        std::find(compiledApis.begin(), compiledApis.end(), RtAudio::WINDOWS_WASAPI)
        != compiledApis.end();

    std::cout << "\nRequired backends:\n";
    std::cout << "  ASIO:   " << (hasAsio ? "available" : "missing") << "\n";
    std::cout << "  WASAPI: " << (hasWasapi ? "available" : "missing") << "\n";

    if (hasAsio)
        listDevicesForApi(RtAudio::WINDOWS_ASIO);

    if (hasWasapi)
        listDevicesForApi(RtAudio::WINDOWS_WASAPI);
}