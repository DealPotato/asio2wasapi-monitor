#include "AudioBridge.h"
#include "Config.h"
#include "DeviceList.h"

#include <chrono>
#include <conio.h>
#include <iostream>
#include <stdexcept>
#include <thread>

static void printStartupInfo(const Config& config)
{
    std::cout << "ASIO2WASAPI Monitor - Bridge MVP\n\n";
    std::cout << "ASIO input device:    " << config.asioDeviceId << "\n";
    std::cout << "ASIO input channel:   " << config.inputChannel << "\n";
    std::cout << "WASAPI output device: " << config.wasapiDeviceId << "\n";
    std::cout << "Sample rate:          " << config.sampleRate << "\n";
    std::cout << "Buffer frames:        " << config.bufferFrames << "\n";
    std::cout << "Prebuffer:            " << config.prebufferMs << " ms\n";
    std::cout << "Gain:                 " << config.gain << "\n";
    std::cout << "Ring samples:         " << config.ringSamples << "\n";
    std::cout << "Press Q to quit.\n\n";
}

static void runConsoleStatusLoop(AudioBridge& bridge)
{
    while (true)
    {
        std::cout << "\x1b[2J\x1b[H";

        std::cout << "ASIO2WASAPI Monitor - Bridge Running\n\n";
        std::cout << "Input peak:       " << bridge.inputPeak() << "\n";
        std::cout << "Ring available:   " << bridge.ringAvailable() << " samples\n";
        std::cout << "Input callbacks:  " << bridge.inputCallbacks() << "\n";
        std::cout << "Output callbacks: " << bridge.outputCallbacks() << "\n";
        std::cout << "Underruns:        " << bridge.underruns() << "\n";
        std::cout << "Overruns:         " << bridge.overruns() << "\n";
        std::cout << "\nPress Q to quit.\n";

        if (_kbhit())
        {
            const int key = _getch();

            if (key == 'q' || key == 'Q')
                break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char** argv)
{
    try
    {
        const Config config = parseArgs(argc, argv);

        if (config.listDevices)
        {
            listDevices();
            return 0;
        }

        printStartupInfo(config);

        AudioBridge bridge(config);
        bridge.start();

        runConsoleStatusLoop(bridge);

        bridge.stop();

        std::cout << "Done.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nError: " << e.what() << "\n";
        return 1;
    }
}