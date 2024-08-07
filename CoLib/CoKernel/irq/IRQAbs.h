#pragma once
#include <vector>
#include "../../CoRo/Generator.h"

class IRQAbs
{
public:
    IRQAbs(int fd, void *icu) :revents_(0), wevents_(0), icu_(icu), fd_(fd) {}
    virtual ~IRQAbs()
    {}
    
    virtual Generator<std::coroutine_handle<>> wakeup() = 0;

    void setREvents(uint revents)
    {
        revents_ = revents;
    }


    void setWEvents(uint wevents)
    {
        wevents_ = wevents;
    }

    void addREvents(uint revents)
    {
        revents_ |= revents;
    }


    void addWEvents(uint wevents)
    {
        wevents_ |= wevents;
    }

    uint getREvents() const
    {
        return revents_;
    }


    uint getWEvents() const
    {
        return wevents_;
    }

    void *getICU() const
    {
        return icu_;
    }

    int getFd() const
    {
        return fd_;
    }

    void setWaked(bool waked)
    {
       waked_ = waked;
    }

    bool getWaked() const
    {
        return waked_;
    }

protected:
    /* 读写多线程 */
    uint revents_;
    uint wevents_;
    void *const icu_;
    int fd_;
    bool waked_;
};

using IRQList = std::vector<IRQAbs>;
using IRQPtrList = std::vector<IRQAbs *>;
