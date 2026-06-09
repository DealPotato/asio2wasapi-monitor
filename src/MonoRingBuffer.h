#pragma once

#include <atomic>
#include <cstddef>
#include <stdexcept>
#include <vector>

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