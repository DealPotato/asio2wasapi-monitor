#include "Config.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

void printHelp()
{
    std::cout
        << "ASIO2WASAPI Monitor\n\n"
        << "Usage:\n"
        << "  asio2wasapi-monitor.exe [options]\n\n"
        << "Options:\n"
        << "  --list               List ASIO and WASAPI devices, then exit.\n"
        << "  --asio <id>          ASIO input device id. Default: 130\n"
        << "  --wasapi <id>        WASAPI output device id. Default: 131\n"
        << "  --channel <n>        ASIO input channel, 1-based. Default: 2\n"
        << "  --rate <hz>          Sample rate. Default: 48000\n"
        << "  --buffer <frames>    Buffer size. Default: 128\n"
        << "  --prebuffer <ms>     Prebuffer before output starts. Default: 5\n"
        << "  --gain <value>       Output gain. Default: 1.0\n"
        << "  --ring <samples>     Ring buffer size in samples. Default: 8192\n"
        << "  --help               Show this help.\n\n"
        << "Example:\n"
        << "  asio2wasapi-monitor.exe --asio 130 --wasapi 131 --channel 2 --buffer 64 --prebuffer 1 --gain 1.0\n";
}

static unsigned int parseUInt(const std::string& value, const std::string& name)
{
    try
    {
        const unsigned long parsed = std::stoul(value);

        if (parsed == 0)
            throw std::runtime_error(name + " must be greater than zero.");

        return static_cast<unsigned int>(parsed);
    }
    catch (...)
    {
        throw std::runtime_error("Invalid value for " + name + ": " + value);
    }
}

static unsigned int parseUIntAllowZero(const std::string& value, const std::string& name)
{
    try
    {
        const unsigned long parsed = std::stoul(value);
        return static_cast<unsigned int>(parsed);
    }
    catch (...)
    {
        throw std::runtime_error("Invalid value for " + name + ": " + value);
    }
}

static float parseFloat(const std::string& value, const std::string& name)
{
    try
    {
        return std::stof(value);
    }
    catch (...)
    {
        throw std::runtime_error("Invalid value for " + name + ": " + value);
    }
}

static std::size_t nextPowerOfTwo(std::size_t value)
{
    if (value < 2)
        return 2;

    --value;

    for (std::size_t shift = 1; shift < sizeof(std::size_t) * 8; shift <<= 1)
        value |= value >> shift;

    return value + 1;
}

Config parseArgs(int argc, char** argv)
{
    Config config;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        auto requireValue = [&](const std::string& option) -> std::string
        {
            if (i + 1 >= argc)
                throw std::runtime_error("Missing value for " + option);

            return argv[++i];
        };

        if (arg == "--help" || arg == "-h")
        {
            printHelp();
            std::exit(0);
        }
        else if (arg == "--list")
        {
            config.listDevices = true;
        }
        else if (arg == "--asio")
        {
            config.asioDeviceId = parseUInt(requireValue(arg), arg);
        }
        else if (arg == "--wasapi")
        {
            config.wasapiDeviceId = parseUInt(requireValue(arg), arg);
        }
        else if (arg == "--channel")
        {
            config.inputChannel = parseUInt(requireValue(arg), arg);
        }
        else if (arg == "--rate")
        {
            config.sampleRate = parseUInt(requireValue(arg), arg);
        }
        else if (arg == "--buffer")
        {
            config.bufferFrames = parseUInt(requireValue(arg), arg);
        }
        else if (arg == "--prebuffer")
        {
            config.prebufferMs = parseUIntAllowZero(requireValue(arg), arg);
        }
        else if (arg == "--gain")
        {
            config.gain = parseFloat(requireValue(arg), arg);
        }
        else if (arg == "--ring")
        {
            config.ringSamples = parseUInt(requireValue(arg), arg);
        }
        else
        {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    config.ringSamples = nextPowerOfTwo(config.ringSamples);

    if (config.gain < 0.0f)
        throw std::runtime_error("--gain cannot be negative.");

    return config;
}