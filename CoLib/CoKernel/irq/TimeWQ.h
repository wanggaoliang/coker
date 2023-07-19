
#pragma once
#include "../../utils/Common.h"
#include "IRQAbs.h"
#include <chrono>
#include <list>

using TimePoint = std::chrono::steady_clock::time_point;
using TimeInterval = std::chrono::microseconds;
using TimeCBWrap = std::function<void()>;
class TimeWQ :public IRQAbs
{
public:

    using TimerId = uint64_t;
    struct WaitItem
    {
        std::coroutine_handle<> h_;
        TimeCBWrap cb_;
        TimePoint when_;

        WaitItem(const TimePoint &when) :when_(when)
        {
            
        }
        
        WaitItem(const std::coroutine_handle<> &h, const TimePoint &when, TimeCBWrap &&cb) :
            h_(h),
            when_(when),
            cb_(std::move(cb))
        {
            
        }
    };

    TimeWQ(void *icu);

    TimeWQ(int fd, void *icu) :IRQAbs(fd, icu) {}

    ~TimeWQ();

    Generator<std::coroutine_handle<>> wakeup() override;

    std::list<WaitItem>::iterator addWait(const std::coroutine_handle<> &, const TimePoint &, TimeCBWrap &&);
    
    std::list<WaitItem>::iterator addWait(const TimePoint &);

    void delWait(std::list<WaitItem>::iterator it)
    {
        items_.erase(it);
    }

private:
    int readTimerfd();

    int resetTimerfd(const TimePoint &expiration);
    std::list<WaitItem> items_;
    
};