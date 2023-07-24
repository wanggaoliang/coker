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

template<typename Res>
struct ICUFAwaiter
{
    using ret_type = Res;

    template <typename F>
    ICUFAwaiter(ICU *icu, F &&func) : func_(std::forward<F>(func)), icu_(icu)
    {}

    bool await_ready() noexcept
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> handle)
    {
        icu_->queueInLoop([h = std::move(handle), func = std::move(func_), this]() mutable -> void {
            ret = func();
            CoKernel::getKernel()->wakeUpReady(h);
                           });
    }

    ret_type await_resume() noexcept
    {
        return ret;
    }

private:
    ICU *icu_;
    ret_type ret;
    std::function<ret_type()> func_;
};

template <>
struct ICUFAwaiter<void>
{
    using ret_type = void;

    template <typename F>
    ICUFAwaiter(ICU *icu, F &&func) : func_(std::forward<F>(func)), icu_(icu)
    {}

    bool await_ready() const noexcept
    {
       return false;
    }

    void await_suspend(std::coroutine_handle<> handle)
    {
        icu_->queueInLoop([h = std::move(handle), func = std::move(func_)]() mutable -> void {
            func();
            CoKernel::getKernel()->wakeUpReady(h);
                           });
    }

    void await_resume() noexcept
    {}

private:
    ICU *icu_;
    std::function <void()> func_;
};

template <typename F>
    requires std::invocable<F>
ICUFAwaiter(ICU* icu, F &&func)->ICUFAwaiter<decltype(std::declval<F>()())>;

struct TimeAwaiter
{
    TimePoint point_;
    ICU *icu_;
    TimeAwaiter(const TimePoint &when,ICU *icu) : point_(when),icu_(icu){};

    ~TimeAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> h)
    {
        icu_->queueInLoop([h, when = point_, icu = icu_]() mutable -> void {
            auto timer = icu->getTimerWQ();
            auto it = timer->addWait(when);it->h_ = h;
                         });
        return true;
    }

    void await_resume() noexcept
    {}
};

struct FileAwaiter
{
    template <typename F>
    requires std::is_convertible_v<F,std::function<ioret(int,uint)>>
    FileAwaiter(std::shared_ptr<FileWQ> &fq, uint events, F &&func, std::chrono::microseconds &ti) : func_(std::forward<F>(func)), fq_(fq), events_(events), ti_(ti), err_(0)
    {}

    ~FileAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle)
    {
        auto icu = static_cast<ICU *>(fq_->getICU());
        auto xFunc = [this](int fd,uint events)mutable->bool {
            auto cons = this->func_(fd, events);
            ret = cons.ret;
            err_ = errno;
            return cons.block;
        };
        icu->queueInLoop([h = std::move(handle), func = std::move(xFunc),icu,this]() mutable -> void {
        auto revents = fq_->getREvents();
        auto bl = func(fq_->getFd(), 0);
        fq_->setREvents(revents & ~ events_);
        if (bl)
        {
            if (ti_.count())
            {
                std::cout << "add wait2" << std::endl;
                auto timer = icu->getTimerWQ();
                auto it = timer->addWait(std::chrono::steady_clock::now()+ti_);
                it->h_ = h;
                auto cb = [func, it, timer](int fd, uint events)mutable {
                    func(fd, events);
                    std::cout << "file del timeout" << std::endl;
                    timer->delWait(it);
                    };
                auto fit = fq_->addWait(h, events_, std::move(cb));
                auto fcb = [fit, this]()mutable {
                    std::cout << "timeout del file" << std::endl;
                    ret = -1;
                    err_ = EWOULDBLOCK;
                    fq_->delWait(fit);
                    };
                it->cb_ = std::move(fcb);
            }
            else
            {
                fq_->addWait(h, events_, std::move(func));
            }
        }
        else
        {
            CoKernel::getKernel()->wakeUpReady(h);
        }
        });
        return true;
    }

    int await_resume() noexcept
    {
        errno = err_;
        return ret;
    }

private:
    int ret;
    int err_;
    std::shared_ptr<FileWQ> fq_;
    uint events_;
    std::chrono::microseconds ti_;
    std::function<ioret(int, uint)> func_;
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