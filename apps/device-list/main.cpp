#include "RtAudio.h"

#include <iostream>
#include <string>

namespace
{
    void listApi(RtAudio::Api api, const char* prefix)
    {
        try
        {
            RtAudio audio(api);

            for (const auto deviceId : audio.getDeviceIds())
            {
                try
                {
                    const auto info = audio.getDeviceInfo(deviceId);

                    if (std::string(prefix) == "ASIO_INPUT")
                    {
                        if (info.inputChannels == 0)
                            continue;

                        if (info.name.find("ASIO2WASAPI") != std::string::npos)
                            continue;

                        std::cout
                            << "ASIO_INPUT|"
                            << info.name
                            << "|"
                            << info.inputChannels
                            << "\n";
                    }

                    if (std::string(prefix) == "WASAPI_OUTPUT")
                    {
                        if (info.outputChannels == 0)
                            continue;

                        std::cout
                            << "WASAPI_OUTPUT|"
                            << info.name
                            << "|"
                            << info.outputChannels
                            << "\n";
                    }
                }
                catch (...)
                {
                }
            }
        }
        catch (...)
        {
        }
    }
}

int main()
{
    listApi(RtAudio::WINDOWS_ASIO, "ASIO_INPUT");
    listApi(RtAudio::WINDOWS_WASAPI, "WASAPI_OUTPUT");

    return 0;
}