#include <RtAudio.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <conio.h>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>

class MonoRingBuffer
{
public:
    explicit MonoRingBuffer(size_t capacityPowerOfTwo)
        : buffer_(capacityPowerOfTwo), mask_(capacityPowerOfTwo - 1)
    {
        if ((capacityPowerOfTwo & (capacityPowerOfTwo - 1)) != 0)
            throw std::runtime_error("Ring buffer capacity must be a power of two.");
    }

    size_t write(const float* input, size_t count)
    {
        const size_t read = readIndex_.load(std::memory_order_acquire);
        const size_t write = writeIndex_.load(std::memory_order_relaxed);

        const size_t used = write - read;
        const size_t freeSpace = buffer_.size() - used;
        const size_t toWrite = std::min(count, freeSpace);

        for (size_t i = 0; i < toWrite; ++i)
            buffer_[(write + i) & mask_] = input[i];

        writeIndex_.store(write + toWrite, std::memory_order_release);
        return toWrite;
    }

    size_t read(float* output, size_t count)
    {
        const size_t write = writeIndex_.load(std::memory_order_acquire);
        const size_t read = readIndex_.load(std::memory_order_relaxed);

        const size_t available = write - read;
        const size_t toRead = std::min(count, available);

        for (size_t i = 0; i < toRead; ++i)
            output[i] = buffer_[(read + i) & mask_];

        readIndex_.store(read + toRead, std::memory_order_release);
        return toRead;
    }

    size_t available() const
    {
        const size_t write = writeIndex_.load(std::memory_order_acquire);
        const size_t read = readIndex_.load(std::memory_order_acquire);
        return write - read;
    }

private:
    std::vector<float> buffer_;
    size_t mask_;

    std::atomic<size_t> readIndex_{0};
    std::atomic<size_t> writeIndex_{0};
};

struct BridgeState
{
    MonoRingBuffer ring{8192};

    std::atomic<float> inputPeak{0.0f};
    std::atomic<unsigned long long> inputCallbacks{0};
    std::atomic<unsigned long long> outputCallbacks{0};
    std::atomic<unsigned long long> underruns{0};
    std::atomic<unsigned long long> overruns{0};

    float gain = 1.0f;
};

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

    std::vector<float> mono(nFrames);

    float peak = 0.0f;

    for (unsigned int i = 0; i < nFrames; ++i)
    {
        // Scarlett ASIO input 2.
        const float sample = input[i * 2 + 1];

        mono[i] = sample;

        const float absSample = std::fabs(sample);
        if (absSample > peak)
            peak = absSample;
    }

    const size_t written = state->ring.write(mono.data(), nFrames);

    if (written < nFrames)
        state->overruns.fetch_add(1, std::memory_order_relaxed);

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

    std::vector<float> mono(nFrames);
    const size_t read = state->ring.read(mono.data(), nFrames);

    if (read < nFrames)
        state->underruns.fetch_add(1, std::memory_order_relaxed);

    for (unsigned int i = 0; i < nFrames; ++i)
    {
        float sample = 0.0f;

        if (i < read)
            sample = mono[i] * state->gain;

        // Mono guitar to stereo headphones.
        output[i * 2 + 0] = sample;
        output[i * 2 + 1] = sample;
    }

    state->outputCallbacks.fetch_add(1, std::memory_order_relaxed);

    return 0;
}

int main()
{
    constexpr unsigned int asioInputDeviceId = 130;    // Focusrite USB ASIO
    constexpr unsigned int wasapiOutputDeviceId = 131; // Headphones (3- Arctis 7+)

    constexpr unsigned int sampleRate = 48000;
    unsigned int bufferFrames = 128;

    std::cout << "ASIO2WASAPI Monitor - Bridge MVP\n\n";
    std::cout << "ASIO input device:   " << asioInputDeviceId << " Focusrite USB ASIO\n";
    std::cout << "ASIO input channel:  2\n";
    std::cout << "WASAPI output device:" << wasapiOutputDeviceId << " Arctis 7+\n";
    std::cout << "Sample rate:         " << sampleRate << "\n";
    std::cout << "Buffer frames:       " << bufferFrames << "\n";
    std::cout << "Press Q to quit.\n\n";

    BridgeState state;

    try
    {
        RtAudio asioInput(RtAudio::WINDOWS_ASIO);
        RtAudio wasapiOutput(RtAudio::WINDOWS_WASAPI);

        RtAudio::StreamParameters inputParams;
        inputParams.deviceId = asioInputDeviceId;
        inputParams.nChannels = 2;
        inputParams.firstChannel = 0;

        RtAudio::StreamParameters outputParams;
        outputParams.deviceId = wasapiOutputDeviceId;
        outputParams.nChannels = 2;
        outputParams.firstChannel = 0;

        RtAudio::StreamOptions inputOptions;
        inputOptions.flags = RTAUDIO_MINIMIZE_LATENCY;
        inputOptions.streamName = "ASIO2WASAPI ASIO Input";

        RtAudio::StreamOptions outputOptions;
        outputOptions.flags = RTAUDIO_MINIMIZE_LATENCY;
        outputOptions.streamName = "ASIO2WASAPI WASAPI Output";

        asioInput.openStream(
            nullptr,
            &inputParams,
            RTAUDIO_FLOAT32,
            sampleRate,
            &bufferFrames,
            &asioInputCallback,
            &state,
            &inputOptions
        );

        unsigned int outputBufferFrames = bufferFrames;

        wasapiOutput.openStream(
            &outputParams,
            nullptr,
            RTAUDIO_FLOAT32,
            sampleRate,
            &outputBufferFrames,
            &wasapiOutputCallback,
            &state,
            &outputOptions
        );

        asioInput.startStream();

        // Small pre-buffer so output does not start completely empty.
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

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
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nError: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Done.\n";
    return 0;
}