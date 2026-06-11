#include "DriverConfig.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace
{
    void moduleAnchor()
    {
    }

    std::filesystem::path getModuleDirectory()
    {
        HMODULE module = nullptr;

        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&moduleAnchor),
            &module);

        char path[MAX_PATH] = {};
        GetModuleFileNameA(module, path, MAX_PATH);

        return std::filesystem::path(path).parent_path();
    }

    std::string trim(std::string value)
    {
        auto isSpace = [](unsigned char c) {
            return std::isspace(c) != 0;
        };

        value.erase(
            value.begin(),
            std::find_if(value.begin(), value.end(), [&](char c) {
                return !isSpace(static_cast<unsigned char>(c));
            }));

        value.erase(
            std::find_if(value.rbegin(), value.rend(), [&](char c) {
                return !isSpace(static_cast<unsigned char>(c));
            }).base(),
            value.end());

        return value;
    }

    void writeDefaultConfig(const std::filesystem::path& path)
    {
        std::ofstream file(path);

        if (!file)
            return;

        file
            << "[Audio]\n"
            << "sampleRate=48000\n"
            << "wasapiBufferFrames=256\n"
            << "inputRingFrames=2048\n"
            << "outputRingFrames=2048\n"
            << "\n"
            << "[Input]\n"
            << "preferredAsioInputDevice=Focusrite\n"
            << "; zero-based: 0 = Input 1, 1 = Input 2\n"
            << "hardwareInputChannel=1\n"
            << "inputGain=1.0\n"
            << "enableTestTone=false\n"
            << "\n"
            << "[Output]\n"
            << "useDefaultWasapiDevice=true\n"
            << "preferredWasapiDevice=\n"
            << "outputGain=1.0\n"
            << "\n"
            << "[Debug]\n"
            << "enableLogging=true\n";
    }

    std::unordered_map<std::string, std::string> parseIni(
        const std::filesystem::path& path)
    {
        std::unordered_map<std::string, std::string> values;

        std::ifstream file(path);

        if (!file)
            return values;

        std::string section;
        std::string line;

        while (std::getline(file, line))
        {
            line = trim(line);

            if (line.empty())
                continue;

            if (line[0] == ';' || line[0] == '#')
                continue;

            if (line.front() == '[' && line.back() == ']')
            {
                section = trim(line.substr(1, line.size() - 2));
                continue;
            }

            const auto equals = line.find('=');

            if (equals == std::string::npos)
                continue;

            const std::string key = trim(line.substr(0, equals));
            const std::string value = trim(line.substr(equals + 1));

            values[section + "." + key] = value;
        }

        return values;
    }

    std::string getString(
        const std::unordered_map<std::string, std::string>& values,
        const std::string& key,
        const std::string& fallback)
    {
        const auto it = values.find(key);

        if (it == values.end())
            return fallback;

        return it->second;
    }

    unsigned int getUInt(
        const std::unordered_map<std::string, std::string>& values,
        const std::string& key,
        unsigned int fallback)
    {
        const auto it = values.find(key);

        if (it == values.end())
            return fallback;

        try
        {
            return static_cast<unsigned int>(std::stoul(it->second));
        }
        catch (...)
        {
            return fallback;
        }
    }

    float getFloat(
        const std::unordered_map<std::string, std::string>& values,
        const std::string& key,
        float fallback)
    {
        const auto it = values.find(key);

        if (it == values.end())
            return fallback;

        try
        {
            return std::stof(it->second);
        }
        catch (...)
        {
            return fallback;
        }
    }

    bool getBool(
        const std::unordered_map<std::string, std::string>& values,
        const std::string& key,
        bool fallback)
    {
        const auto it = values.find(key);

        if (it == values.end())
            return fallback;

        std::string value = it->second;

        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

        if (value == "1" || value == "true" || value == "yes" || value == "on")
            return true;

        if (value == "0" || value == "false" || value == "no" || value == "off")
            return false;

        return fallback;
    }
}

std::filesystem::path DriverConfig::configPath()
{
    return getModuleDirectory() / "asio2wasapi-monitor.ini";
}

std::filesystem::file_time_type DriverConfig::lastWriteTime()
{
    try
    {
        const auto path = configPath();

        if (!std::filesystem::exists(path))
            return {};

        return std::filesystem::last_write_time(path);
    }
    catch (...)
    {
        return {};
    }
}

DriverConfig DriverConfig::load()
{
    DriverConfig config;

    const auto path = configPath();

    if (!std::filesystem::exists(path))
    {
        writeDefaultConfig(path);
        return config;
    }

    const auto values = parseIni(path);

    config.sampleRate = getUInt(values, "Audio.sampleRate", config.sampleRate);
    config.wasapiBufferFrames = getUInt(values, "Audio.wasapiBufferFrames", config.wasapiBufferFrames);
    config.inputRingFrames = getUInt(values, "Audio.inputRingFrames", config.inputRingFrames);
    config.outputRingFrames = getUInt(values, "Audio.outputRingFrames", config.outputRingFrames);

    config.preferredAsioInputDevice = getString(values, "Input.preferredAsioInputDevice", config.preferredAsioInputDevice);
    config.hardwareInputChannel = getUInt(values, "Input.hardwareInputChannel", config.hardwareInputChannel);
    config.inputGain = getFloat(values, "Input.inputGain", config.inputGain);
    config.enableTestTone = getBool(values, "Input.enableTestTone", config.enableTestTone);

    config.useDefaultWasapiDevice = getBool(values, "Output.useDefaultWasapiDevice", config.useDefaultWasapiDevice);
    config.preferredWasapiDevice = getString(values, "Output.preferredWasapiDevice", config.preferredWasapiDevice);
    config.outputGain = getFloat(values, "Output.outputGain", config.outputGain);

    config.enableLogging = getBool(values, "Debug.enableLogging", config.enableLogging);

    return config;
}