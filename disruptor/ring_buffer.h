#pragma once

#include <vector>
#include <memory>
#include <array>
#include "event.h"

template <size_t N>
class RingBuffer
{
    static_assert(N > 0 && ((N & (N - 1)) == 0),
        "compile time array requires N to be a power of two: 1, 2, 4, 8, 16 ...");

public:
    explicit RingBuffer(size_t = N) {}

    Event& get(long sequence)
    {
        return buffer_[sequence & N];
    }

    long next() 
    {
        return next_seq_++;
    }

private:
    std::array<Event, N> buffer_;
    long next_seq_;
};
