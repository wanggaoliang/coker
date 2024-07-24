#pragma once
#include <coroutine>
#include <atomic>
#include <chrono>
#include "../utils/MpmcQueue.h"
struct CvCore
{
    using TimePoint = std::chrono::steady_clock::time_point;
    struct WaitItem
    {
        std::coroutine_handle<> h_;
        TimePoint when_; 
    };
    std::atomic<uint> margin__;
    uint index;
    MpmcQueue<WaitItem> items_;
};