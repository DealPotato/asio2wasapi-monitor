#pragma once

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <vector>
#include <cstdint>

class StereoRingBuffer
{
public:
    explicit StereoRingBuffer(std::size_t capacityFrames)
        : buffer_(capacityFrames * 2, 0.0f),
          capacityFrames_(capacityFrames)
    {
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        readFrame_ = 0;
        writeFrame_ = 0;
        availableFrames_ = 0;
        underrunFrames_ = 0;
        droppedFrames_ = 0;

        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

    std::size_t availableFrames() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return availableFrames_;
    }

    std::size_t writeFromPlanar(
        const float* left,
        const float* right,
        std::size_t frames)
    {
        if (!left || !right || frames == 0 || capacityFrames_ == 0)
            return 0;

        std::lock_guard<std::mutex> lock(mutex_);

        std::size_t sourceOffset = 0;
        std::size_t framesToWrite = frames;

        if (framesToWrite > capacityFrames_)
        {
            sourceOffset = framesToWrite - capacityFrames_;
            framesToWrite = capacityFrames_;
        }

        const std::size_t freeFrames = capacityFrames_ - availableFrames_;

        if (framesToWrite > freeFrames)
        {
            const std::size_t framesToDrop = framesToWrite - freeFrames;

            readFrame_ = (readFrame_ + framesToDrop) % capacityFrames_;
            availableFrames_ -= framesToDrop;
            droppedFrames_ += static_cast<std::uint64_t>(framesToDrop);
        }

        for (std::size_t i = 0; i < framesToWrite; ++i)
        {
            const std::size_t index = ((writeFrame_ + i) % capacityFrames_) * 2;
            const std::size_t sourceIndex = sourceOffset + i;

            buffer_[index + 0] = left[sourceIndex];
            buffer_[index + 1] = right[sourceIndex];
        }

        writeFrame_ = (writeFrame_ + framesToWrite) % capacityFrames_;
        availableFrames_ += framesToWrite;

        return framesToWrite;
    }

    std::size_t readInterleaved(float* output, std::size_t frames)
    {
        if (!output || frames == 0)
            return 0;

        std::lock_guard<std::mutex> lock(mutex_);

        const std::size_t framesToRead = std::min(frames, availableFrames_);

        if (framesToRead < frames)
        {
            underrunFrames_ += static_cast<std::uint64_t>(frames - framesToRead);
        }

        for (std::size_t i = 0; i < framesToRead; ++i)
        {
            const std::size_t index = ((readFrame_ + i) % capacityFrames_) * 2;

            output[(i * 2) + 0] = buffer_[index + 0];
            output[(i * 2) + 1] = buffer_[index + 1];
        }

        for (std::size_t i = framesToRead; i < frames; ++i)
        {
            output[(i * 2) + 0] = 0.0f;
            output[(i * 2) + 1] = 0.0f;
        }

        readFrame_ = (readFrame_ + framesToRead) % capacityFrames_;
        availableFrames_ -= framesToRead;

        return framesToRead;
    }

    std::uint64_t underrunFrames() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return underrunFrames_;
    }

    std::uint64_t droppedFrames() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return droppedFrames_;
    }

private:
    mutable std::mutex mutex_;

    std::vector<float> buffer_;
    std::size_t capacityFrames_ = 0;
    std::size_t readFrame_ = 0;
    std::size_t writeFrame_ = 0;
    std::size_t availableFrames_ = 0;
    std::uint64_t underrunFrames_ = 0;
    std::uint64_t droppedFrames_ = 0;
};