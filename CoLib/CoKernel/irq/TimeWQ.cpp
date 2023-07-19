#include "TimeWQ.h"
#include <chrono>
#include <sys/timerfd.h>
#include <memory.h>
#include <unistd.h>

TimeWQ::TimeWQ(void *icu) :IRQAbs(-1,icu)
{
    fd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
}

TimeWQ::~TimeWQ()
{
    readTimerfd();
    
    if (fd_ >= 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
}

Generator<std::coroutine_handle<>> TimeWQ::wakeup()
{
    const auto now = std::chrono::steady_clock::now();
    readTimerfd();
    for (auto iter = items_.begin(); iter != items_.end(); )
    {
        if (iter->when_ > now)
        {
            resetTimerfd(iter->when_);
            break;
        }
        if (iter->cb_)
        {
            iter->cb_();
        }
        auto h = iter->h_;
        iter = items_.erase(iter);
        if (h)
        {
            co_yield h;
        }
    }
}

int TimeWQ::readTimerfd()
{
    uint64_t howmany;
    ssize_t n = ::read(fd_, &howmany, sizeof howmany);
    if (n != sizeof howmany)
    {
        howmany = -1;
    }
    return howmany;
}

int TimeWQ::resetTimerfd(const TimePoint &expiration)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    auto microSeconds = std::chrono::duration_cast<std::chrono::microseconds>(
        expiration - std::chrono::steady_clock::now()).count();
    memset(&newValue, 0, sizeof(newValue));
    memset(&oldValue, 0, sizeof(oldValue));

    if (microSeconds < 100)
    {
        microSeconds = 100;
    }

    newValue.it_value.tv_sec = static_cast<time_t>(microSeconds / 1000000);
    newValue.it_value.tv_nsec = static_cast<long>((microSeconds % 1000000) * 1000);
    int ret = ::timerfd_settime(fd_, 0, &newValue, &oldValue);
    return ret;
}

std::list<TimeWQ::WaitItem>::iterator TimeWQ::addWait(const std::coroutine_handle<> &h, const TimePoint &when, TimeCBWrap &&cb)
{
    auto iter = items_.begin();
    for (; iter != items_.end(); iter++)
    {

        if (iter->when_ > when)
        {
            break;
        }  
    }
    if (iter == items_.begin())
    {
        resetTimerfd(when);
    }
    return items_.emplace(iter, h, when,std::move(cb));
    
}

std::list<TimeWQ::WaitItem>::iterator TimeWQ::addWait(const TimePoint &when)
{
    auto iter = items_.begin();
    for (; iter != items_.end(); iter++)
    {

        if (iter->when_ > when)
        {
            break;
        }
    }
    if (iter == items_.begin())
    {
        resetTimerfd(when);
    }
    return items_.emplace(iter, when);

}

