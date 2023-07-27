#pragma once
#include "irq/TimeWQ.h"
#include "irq/FileWQ.h"
#include "CompStarter.h"
#include "MuCore.h"
#include "../CoRo/Lazy.h"
#include <mutex>
#include <memory>
#include <unordered_map>
#include <type_traits>
#include <concepts>
#include <condition_variable>

class CoKernel
{
public:
    using FileWQMap = std::unordered_map<int, std::shared_ptr<FileWQ>>;
    using CompStarterPtr = std::shared_ptr<CompStarter>;

    Lazy<int> waitFile(int, uint32_t, WQCB, std::chrono::microseconds);

    Lazy<void> waitTime(const TimePoint &);

    Lazy<void> CoRoLock(MuCore &);

    Lazy<void> CoRoUnlock(MuCore &);

    Lazy<int> updateIRQ(int, uint32_t);

    Lazy<int> removeIRQ(int);

    void wakeUpReady(std::coroutine_handle<> &);

    bool schedule();

    void start();

    static void INIT(uint icuNum, uint cpuNum)
    {
        kernel = std::shared_ptr<CoKernel>(new CoKernel(icuNum, cpuNum));
    }

    static std::shared_ptr<CoKernel> getKernel()
    {
        return kernel;
    }

    ~CoKernel() = default;
    void print_core();

private:
    CoKernel(uint,uint);
    const uint icuNum_;
    const uint cpuNum_;
    /* 只有主线程使用 */
    std::vector<CompStarterPtr> comps_;
    std::shared_ptr<ICU> icu_;
    std::vector<ICU *> icus_;
    std::vector<CPU *> cpus_;
    
    FileWQMap fileWQPtrs_;
    MuCore fMapLk_;

    std::mutex hmu;
    std::condition_variable hcv;
    MpmcQueue<std::coroutine_handle<>> readyRo_;
    /* 处理全在非协程中 */
    static std::shared_ptr<CoKernel> kernel;
};

using LockHandle = std::coroutine_handle<LazyPromise<void>>;
struct LockAwaiter
{
    MuCore *mu_;

    LockAwaiter(MuCore *mu) :mu_(mu)
    {

    }

    ~LockAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(LockHandle h)
    {
        auto old = mu_->waiter_++;
        if (!old)
        {
            mu_->tid_.store(h.promise()._tcb->tid);
            return false;
        }
        else
        {
            mu_->items_.enqueue(MuCore::WaitItem{ h.promise()._tcb->tid, h });
            return true;
        }
    }

    void await_resume() noexcept
    {}
};

struct TryLockAwaiter
{
    MuCore *mu_;
    bool ret;
    TryLockAwaiter(MuCore *mu) :mu_(mu)
    {

    }

    ~TryLockAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(LockHandle h)
    {
        uint no_wait = 0;
        ret = mu_->waiter_.compare_exchange_strong(no_wait, 1U);
        if (ret)
        {
            mu_->tid_.store(h.promise()._tcb->tid);
        }
        return false;
    }

    bool await_resume() noexcept
    {
        return ret;
    }
};

struct UnlockAwaiter
{
    MuCore *mu_;
    UnlockAwaiter(MuCore *mu) :mu_(mu)
    {

    };

    ~UnlockAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(LockHandle h)
    {
        int ul = 0;
        MuCore::WaitItem newItem;
        auto ret = mu_->tid_.compare_exchange_strong(h.promise()._tcb->tid, 0);
        if (ret)
        {
            auto old = mu_->waiter_--;
            if (old > 1)
            {
                while (!mu_->items_.dequeue(newItem));
                mu_->tid_.store(newItem.tid_);
                CoKernel::getKernel()->wakeUpReady(newItem.h_);
            }
        }
        else
        {
            //throw exception
        }
        return false;
    }

    void await_resume() noexcept
    {}
};