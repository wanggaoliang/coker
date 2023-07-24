#pragma once
#include "Component.h"
#include "IRQPoll.h"
#include "../irq/FileWQ.h"
#include "../irq/TimeWQ.h"
#include "../../utils/MpmcQueue.h"
#include <memory>
#include <vector>
#include <thread>
class ICU : public Component
{
public:

    ICU();
    ~ICU();

    void loop() override;

    void quit() override;

    void wakeUp();

    bool isInLoopThread() const
    {
        return threadId_ == std::this_thread::get_id();
    };

    void assertInLoopThread();

    template <typename T>
        requires std::is_convertible_v<T, XCompFunc>
    void queueInLoop(T &&cb)
    {
        funcs_.enqueue(std::forward<T>(cb));
        if (!isInLoopThread() || !looping_.load(std::memory_order_acquire))
        {
            wakeUp();
        }
    }

    template <typename T>
        requires std::is_convertible_v<T, XCompFunc>
    void runInLoop(T &&cb)
    {
        if (isInLoopThread())
        {
            cb();
        }
        else
        {
            queueInLoop(std::forward<T>(cb));
        }
    }

    int addIRQ(IRQAbs *WQA)
    {
        return poller_->updateIRQ(EPOLL_CTL_ADD, WQA);
    }

    int modIRQ(IRQAbs *WQA)
    {
        return poller_->updateIRQ(EPOLL_CTL_MOD, WQA);
    }

    int delIRQ(IRQAbs *WQA)
    {
        return poller_->updateIRQ(EPOLL_CTL_DEL, WQA);
    }

    TimeWQ *getTimerWQ()
    {
        return timerWQ_.get();
    }

    template<typename T>
        requires std::is_convertible_v<T, WakeCB>
    void setWakeCallback(T &&func)
    {
        wakeCB_ = std::forward<T>(func);
    }
public:
    uint irqn;
    uint pos;
private:
    std::thread::id threadId_;
    std::unique_ptr<IRQPoll> poller_;
    MpmcQueue<XCompFunc> funcs_;
    std::unique_ptr<FileWQ> wakeUpWQ_;
    std::unique_ptr<TimeWQ> timerWQ_;
    WakeCB wakeCB_;
};