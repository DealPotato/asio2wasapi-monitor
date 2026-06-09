#include <RtAudio.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <conio.h>
#include <iomanip>
#include <iostream>
#include <thread>

struct MeterState
{
    std::atomic<float> peakCh1{0.0f};
    std::atomic<float> peakCh2{0.0f};
    std::atomic<float> rmsCh1{0.0f};
    std::atomic<float> rmsCh2{0.0f};
    std::atomic<unsigned long long> callbacks{0};
};

static int inputCallback(
    void* outputBuffer,
    void* inputBuffer,
    unsigned int nFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData)
{
    (void)outputBuffer;
    (void)streamTime;

    auto* state = static_cast<MeterState*>(userData);

    if (status)
        std::cerr << "Stream status warning: " << status << "\n";

    if (!inputBuffer)
        return 0;

    const auto* input = static_cast<const float*>(inputBuffer);

    float peak1 = 0.0f;
    float peak2 = 0.0f;
    double sum1 = 0.0;
    double sum2 = 0.0;

    for (unsigned int i = 0; i < nFrames; ++i)
    {
        const float ch1 = input[i * 2 + 0];
        const float ch2 = input[i * 2 + 1];

        peak1 = std::max(peak1, std::fabs(ch1));
        peak2 = std::max(peak2, std::fabs(ch2));

        sum1 += static_cast<double>(ch1) * ch1;
        sum2 += static_cast<double>(ch2) * ch2;
    }

    state->peakCh1.store(peak1, std::memory_order_relaxed);
    state->peakCh2.store(peak2, std::memory_order_relaxed);
    state->rmsCh1.store(static_cast<float>(std::sqrt(sum1 / nFrames)), std::memory_order_relaxed);
    state->rmsCh2.store(static_cast<float>(std::sqrt(sum2 / nFrames)), std::memory_order_relaxed);
    state->callbacks.fetch_add(1, std::memory_order_relaxed);

    return 0;
}

static void printBar(const char* label, float peak, float rms)
{
    constexpr int width = 30;

    const int peakBars = static_cast<int>(std::min(peak * width * 4.0f, static_cast<float>(width)));

    std::cout << label << " [";

    for (int i = 0; i < width; ++i)
        std::cout << (i < peakBars ? "#" : " ");

    std::cout << "] ";

    std::cout << "peak=" << std::fixed << std::setprecision(5) << peak
              << " rms=" << std::fixed << std::setprecision(5) << rms
              << "\n";
}

int main()
{
    constexpr unsigned int focusriteAsioDeviceId = 130;
    constexpr unsigned int sampleRate = 48000;
    unsigned int bufferFrames = 128;

    std::cout << "ASIO2WASAPI Monitor - ASIO Input Meter\n";
    std::cout << "Device: Focusrite USB ASIO [" << focusriteAsioDeviceId << "]\n";
    std::cout << "Sample rate: " << sampleRate << "\n";
    std::cout << "Buffer frames: " << bufferFrames << "\n";
    std::cout << "Reading input channels 1 and 2\n";
    std::cout << "Press Q to quit.\n\n";

    MeterState meter;

    try
    {
        RtAudio audio(RtAudio::WINDOWS_ASIO);

        RtAudio::StreamParameters inputParams;
        inputParams.deviceId = focusriteAsioDeviceId;
        inputParams.nChannels = 2;
        inputParams.firstChannel = 0;

        RtAudio::StreamOptions options;
        options.flags = RTAUDIO_MINIMIZE_LATENCY;
        options.streamName = "ASIO2WASAPI Input Meter";

        audio.openStream(
            nullptr,
            &inputParams,
            RTAUDIO_FLOAT32,
            sampleRate,
            &bufferFrames,
            &inputCallback,
            &meter,
            &options
        );

        audio.startStream();

        while (true)
        {
            std::cout << "\x1b[2J\x1b[H";

            std::cout << "ASIO2WASAPI Monitor - ASIO Input Meter\n\n";

            printBar("Input 1", meter.peakCh1.load(std::memory_order_relaxed),
                               meter.rmsCh1.load(std::memory_order_relaxed));

            printBar("Input 2", meter.peakCh2.load(std::memory_order_relaxed),
                               meter.rmsCh2.load(std::memory_order_relaxed));

            std::cout << "\nCallbacks: " << meter.callbacks.load(std::memory_order_relaxed) << "\n";
            std::cout << "Press Q to quit.\n";

            if (_kbhit())
            {
                const int key = _getch();
                if (key == 'q' || key == 'Q')
                    break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }

        if (audio.isStreamRunning())
            audio.stopStream();

        if (audio.isStreamOpen())
            audio.closeStream();
    }
    catch (const std::exception& e)
    {
        std::cerr << "\nError: " << e.what() << "\n";
        return 1;
    }

    return 0;
}