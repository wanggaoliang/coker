#pragma once
#include "TimeWQ.h"
#include "FileWQ.h"
#include "CompStarter.h"
#include "MuCore.h"
#include "../CoRo/Lazy.h"
#include <mutex>
#include <memory>
#include <unordered_map>
#include <type_traits>
#include <concepts>

class CoKernel
{
public:
    using FileWQMap = std::unordered_map<int, std::shared_ptr<FileWQ>>;
    using CompStarterPtr = std::shared_ptr<CompStarter>;

    Lazy<int> waitFile(int, uint32_t, WQCB);

    Lazy<void> waitTime(const TimePoint &);

    Lazy<void> CoRoLock(MuCore &);

    Lazy<void> CoRoUnlock(MuCore &);

    Lazy<int> updateIRQ(int, uint32_t);

    Lazy<int> removeIRQ(int);

    void wakeUpReady(std::coroutine_handle<> &);

    bool schedule();

    void start();

    static void INIT(uint num)
    {
        kernel = std::shared_ptr<CoKernel>(new CoKernel(num));
    }

    static std::shared_ptr<CoKernel> getKernel()
    {
        return kernel;
    }

    ~CoKernel() = default;
private:
    CoKernel(uint);
    const uint icuNum_;
    const uint cpuNum_;
    /* 只有主线程使用 */
    std::vector<CompStarterPtr> comps_;
    std::shared_ptr<CPU> core_;

    /* 中断注册相关多线程使用，保证线程安全 */

    std::vector<ICU *> icus_;
    FileWQMap fileWQPtrs_;
    std::shared_ptr<TimeWQ> timer_;
    MuCore fMapLk_;

    MpmcQueue<std::coroutine_handle<>> readyRo_;
    /* 处理全在非协程中 */
    static std::shared_ptr<CoKernel> kernel;
};

template<typename Res>
struct XCoreFAwaiter
{
    using ret_type = Res;

    template <typename F>
    XCoreFAwaiter(Core *core, F &&func) : func_(std::forward<F>(func)), core_(core)
    {}

    bool await_ready() noexcept
    {
        if (core_->isInLoopThread())
        {
            ret = func_();
            return true;
        }
        else
        {
            return false;
        }
    }

    void await_suspend(std::coroutine_handle<> handle)
    {
        core_->queueInLoop([h = std::move(handle), func = std::move(func_), this]() mutable -> void {
            ret = func();
            CoKernel::getKernel()->wakeUpReady(h);
                           });
    }

    ret_type await_resume() noexcept
    {
        return ret;
    }

private:
    Core *core_;
    ret_type ret;
    std::function<ret_type()> func_;
};

template <>
struct XCoreFAwaiter<void>
{
    using ret_type = void;

    template <typename F>
    XCoreFAwaiter(Core *core, F &&func) : func_(std::forward<F>(func)), core_(core)
    {}

    bool await_ready() const noexcept
    {
        if (core_->isInLoopThread())
        {
            func_();
            return true;
        }
        else
        {
            return false;
        }
    }

    void await_suspend(std::coroutine_handle<> handle)
    {
        core_->queueInLoop([h = std::move(handle), func = std::move(func_)]() mutable -> void {
            func();
            CoKernel::getKernel()->wakeUpReady(h);
                           });
    }

    void await_resume() noexcept
    {}

private:
    Core *core_;
    std::function <void()> func_;
};

template <typename F>
    requires std::invocable<F>
XCoreFAwaiter(Core* core, F &&func)->XCoreFAwaiter<decltype(std::declval<F>()())>;

struct TimeAwaiter
{
    TimePoint point;
    TimeAwaiter(const TimePoint &when) :point(when)
    {};

    ~TimeAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> h)
    {
        Core::getCurCore()->waitTime(h, point);
        return true;
    }

    void await_resume() noexcept
    {}
};

struct FileAwaiter
{
    template <typename F>
    requires std::is_convertible_v<F,std::function<ioret(int,uint)>>
    FileAwaiter(std::shared_ptr<FileWQ> &fq,uint events,F &&func) : func_(std::forward<F>(func)),fq_(fq),events_(events),err_(0)
    {}

    ~FileAwaiter() = default;

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle)
    {
        auto core = static_cast<Core *>(fq_->getCore());
        auto xFunc = [this](int fd,uint events)mutable->bool {
            auto cons = this->func_(fd, events);
            ret = cons.ret;
            err_ = errno;
            return cons.block;
        };
        if (core->isInLoopThread())
        {
            auto revents = fq_->getREvents();
            auto bl = xFunc(fq_->getFd(), 0);
            fq_->setREvents(revents & ~events_);
            if (bl)
            {
                fq_->addWait(handle, events_, std::move(xFunc));
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            core->queueInLoop([h = std::move(handle), func = std::move(xFunc), this]() mutable -> void {
                auto revents = fq_->getREvents();
                auto bl = func(fq_->getFd(), 0);
                fq_->setREvents(revents & ~ events_);
                if (bl)
                {
                    fq_->addWait(h, events_, std::move(func));
                    fq_->wakeup();
                }
                else
                {
                    CoKernel::getKernel()->wakeUpReady(h);
                }
                });
            return true;
        }
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