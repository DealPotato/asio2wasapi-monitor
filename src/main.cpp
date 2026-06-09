#include <RtAudio.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <conio.h>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

struct Config
{
    unsigned int asioDeviceId = 130;
    unsigned int wasapiDeviceId = 131;
    unsigned int inputChannel = 2; // 1-based user-facing channel.
    unsigned int sampleRate = 48000;
    unsigned int bufferFrames = 128;
    unsigned int prebufferMs = 5;
    float gain = 1.0f;
    std::size_t ringSamples = 8192;
};

static void printHelp()
{
    std::cout
        << "ASIO2WASAPI Monitor\n\n"
        << "Usage:\n"
        << "  asio2wasapi-monitor.exe [options]\n\n"
        << "Options:\n"
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
        << "  asio2wasapi-monitor.exe --asio 130 --wasapi 131 --channel 2 --buffer 64 --prebuffer 5 --gain 1.0\n";
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

static Config parseArgs(int argc, char** argv)
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

class MonoRingBuffer
{
public:
    explicit MonoRingBuffer(std::size_t capacityPowerOfTwo)
        : buffer_(capacityPowerOfTwo), mask_(capacityPowerOfTwo - 1)
    {
        if ((capacityPowerOfTwo & (capacityPowerOfTwo - 1)) != 0)
            throw std::runtime_error("Ring buffer capacity must be a power of two.");
    }

    bool writeOne(float sample)
    {
        const std::size_t read = readIndex_.load(std::memory_order_acquire);
        const std::size_t write = writeIndex_.load(std::memory_order_relaxed);

        if ((write - read) >= buffer_.size())
            return false;

        buffer_[write & mask_] = sample;
        writeIndex_.store(write + 1, std::memory_order_release);
        return true;
    }

    bool readOne(float& sample)
    {
        const std::size_t write = writeIndex_.load(std::memory_order_acquire);
        const std::size_t read = readIndex_.load(std::memory_order_relaxed);

        if (write == read)
            return false;

        sample = buffer_[read & mask_];
        readIndex_.store(read + 1, std::memory_order_release);
        return true;
    }

    std::size_t available() const
    {
        const std::size_t write = writeIndex_.load(std::memory_order_acquire);
        const std::size_t read = readIndex_.load(std::memory_order_acquire);
        return write - read;
    }

private:
    std::vector<float> buffer_;
    std::size_t mask_;

    std::atomic<std::size_t> readIndex_{0};
    std::atomic<std::size_t> writeIndex_{0};
};

struct BridgeState
{
    explicit BridgeState(const Config& cfg)
        : config(cfg), ring(cfg.ringSamples)
    {
    }

    Config config;
    MonoRingBuffer ring;

    std::atomic<float> inputPeak{0.0f};
    std::atomic<unsigned long long> inputCallbacks{0};
    std::atomic<unsigned long long> outputCallbacks{0};
    std::atomic<unsigned long long> underruns{0};
    std::atomic<unsigned long long> overruns{0};
};

static std::size_t calculatePrebufferSamples(const Config& config)
{
    if (config.prebufferMs == 0)
        return 0;

    const auto samplesFromMs =
        static_cast<std::size_t>(
            (static_cast<unsigned long long>(config.sampleRate) * config.prebufferMs) / 1000
        );

    return std::max<std::size_t>(samplesFromMs, config.bufferFrames);
}

static void waitForPrebuffer(BridgeState& state)
{
    const std::size_t targetSamples = calculatePrebufferSamples(state.config);

    if (targetSamples == 0)
        return;

    std::cout << "Waiting for prebuffer: " << targetSamples << " samples...\n";

    const auto start = std::chrono::steady_clock::now();

    while (state.ring.available() < targetSamples)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        const auto elapsed = std::chrono::steady_clock::now() - start;

        if (elapsed > std::chrono::milliseconds(1000))
        {
            std::cout << "Warning: prebuffer timeout. Starting output anyway.\n";
            break;
        }
    }

    std::cout << "Prebuffer ready: " << state.ring.available() << " samples.\n";
}

static int asioInputCallback(
    void* outputBuffer,
    void* inputBuffer,
    unsigned int nFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData)
{
    (void)outputBuffer;
    (void)streamTime;

    auto* state = static_cast<BridgeState*>(userData);

    if (status)
        state->underruns.fetch_add(1, std::memory_order_relaxed);

    if (!inputBuffer)
        return 0;

    const auto* input = static_cast<const float*>(inputBuffer);

    const unsigned int requestedChannels = state->config.inputChannel;
    const unsigned int selectedIndex = state->config.inputChannel - 1;

    float peak = 0.0f;

    for (unsigned int i = 0; i < nFrames; ++i)
    {
        const float sample = input[i * requestedChannels + selectedIndex];

        if (!state->ring.writeOne(sample))
            state->overruns.fetch_add(1, std::memory_order_relaxed);

        peak = std::max(peak, std::fabs(sample));
    }

    state->inputPeak.store(peak, std::memory_order_relaxed);
    state->inputCallbacks.fetch_add(1, std::memory_order_relaxed);

    return 0;
}

static int wasapiOutputCallback(
    void* outputBuffer,
    void* inputBuffer,
    unsigned int nFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData)
{
    (void)inputBuffer;
    (void)streamTime;

    auto* state = static_cast<BridgeState*>(userData);

    if (status)
        state->underruns.fetch_add(1, std::memory_order_relaxed);

    auto* output = static_cast<float*>(outputBuffer);

    for (unsigned int i = 0; i < nFrames; ++i)
    {
        float sample = 0.0f;

        if (!state->ring.readOne(sample))
            state->underruns.fetch_add(1, std::memory_order_relaxed);

        sample *= state->config.gain;

        output[i * 2 + 0] = sample;
        output[i * 2 + 1] = sample;
    }

    state->outputCallbacks.fetch_add(1, std::memory_order_relaxed);

    return 0;
}

int main(int argc, char** argv)
{
    try
    {
        const Config config = parseArgs(argc, argv);

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

        BridgeState state(config);

        RtAudio asioInput(RtAudio::WINDOWS_ASIO);
        RtAudio wasapiOutput(RtAudio::WINDOWS_WASAPI);

        RtAudio::StreamParameters inputParams;
        inputParams.deviceId = config.asioDeviceId;
        inputParams.nChannels = config.inputChannel;
        inputParams.firstChannel = 0;

        RtAudio::StreamParameters outputParams;
        outputParams.deviceId = config.wasapiDeviceId;
        outputParams.nChannels = 2;
        outputParams.firstChannel = 0;

        RtAudio::StreamOptions inputOptions;
        inputOptions.flags = RTAUDIO_MINIMIZE_LATENCY;
        inputOptions.streamName = "ASIO2WASAPI ASIO Input";

        RtAudio::StreamOptions outputOptions;
        outputOptions.flags = RTAUDIO_MINIMIZE_LATENCY;
        outputOptions.streamName = "ASIO2WASAPI WASAPI Output";

        unsigned int inputBufferFrames = config.bufferFrames;

        asioInput.openStream(
            nullptr,
            &inputParams,
            RTAUDIO_FLOAT32,
            config.sampleRate,
            &inputBufferFrames,
            &asioInputCallback,
            &state,
            &inputOptions
        );

        unsigned int outputBufferFrames = config.bufferFrames;

        wasapiOutput.openStream(
            &outputParams,
            nullptr,
            RTAUDIO_FLOAT32,
            config.sampleRate,
            &outputBufferFrames,
            &wasapiOutputCallback,
            &state,
            &outputOptions
        );

        asioInput.startStream();

        waitForPrebuffer(state);

        wasapiOutput.startStream();

        while (true)
        {
            std::cout << "\x1b[2J\x1b[H";

            std::cout << "ASIO2WASAPI Monitor - Bridge Running\n\n";
            std::cout << "Input peak:       " << state.inputPeak.load(std::memory_order_relaxed) << "\n";
            std::cout << "Ring available:   " << state.ring.available() << " samples\n";
            std::cout << "Input callbacks:  " << state.inputCallbacks.load(std::memory_order_relaxed) << "\n";
            std::cout << "Output callbacks: " << state.outputCallbacks.load(std::memory_order_relaxed) << "\n";
            std::cout << "Underruns:        " << state.underruns.load(std::memory_order_relaxed) << "\n";
            std::cout << "Overruns:         " << state.overruns.load(std::memory_order_relaxed) << "\n";
            std::cout << "\nPress Q to quit.\n";

            if (_kbhit())
            {
                const int key = _getch();
                if (key == 'q' || key == 'Q')
                    break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (wasapiOutput.isStreamRunning())
            wasapiOutput.stopStream();

        if (asioInput.isStreamRunning())
            asioInput.stopStream();

        if (wasapiOutput.isStreamOpen())
            wasapiOutput.closeStream();

        if (asioInput.isStreamOpen())
            asioInput.closeStream();

        std::cout << "Done.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nError: " << e.what() << "\n";
        return 1;
    }
}