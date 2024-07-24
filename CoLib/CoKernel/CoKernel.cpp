#include "CoKernel.h"
#include <iostream>
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
            CoKernel::getKernel()->wakeUpReady(h, std::string("ICUF"));
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
            CoKernel::getKernel()->wakeUpReady(h, std::string("ICUFV"));
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
ICUFAwaiter(ICU *icu, F &&func)->ICUFAwaiter<decltype(std::declval<F>()())>;

struct TimeAwaiter
{
    TimePoint point_;
    ICU *icu_;
    TimeAwaiter(const TimePoint &when, ICU *icu) : point_(when), icu_(icu) {};

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
        requires std::is_convertible_v<F, std::function<ioret(int, uint)>>
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
        icu->queueInLoop([h = std::move(handle), icu, this]() mutable -> void {
            if (ti_.count())
            {
                auto timer = icu->getTimerWQ();
                auto it = timer->addWait(std::chrono::steady_clock::now() + ti_);
                it->h_ = h;
                auto cb = [it, timer, this](int fd, uint events)mutable {
                    auto cons = this->func_(fd, events);
                    timer->delWait(it);
                    ret = cons.ret;
                    err_ = errno;
                    return cons;
                    };
                auto fit = fq_->addWait(h, events_, std::move(cb));
                auto fcb = [fit, this]()mutable {
                    ret = -1;
                    err_ = EWOULDBLOCK;
                    fq_->delWait(fit);
                    };
                it->cb_ = std::move(fcb);
            }
            else
            {
                auto cb = [this](int fd, uint events)mutable->ioret {
                    auto cons = this->func_(fd, events);
                    ret = cons.ret;
                    err_ = errno;
                    return cons;
                    };
                fq_->addWait(h, events_, std::move(cb));
            }

            icu->wakeWQ(fq_.get());
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

std::shared_ptr<CoKernel> CoKernel::kernel = nullptr;

CoKernel::CoKernel(uint icuNum, uint cpuNum) :icuNum_(icuNum), cpuNum_(cpuNum)
{
    for (uint i = 0;i < icuNum_ - 1;i++)
    {
        auto cPtr = std::make_shared<CompStarter>(CompStarter::COMP_ICU);
        comps_.push_back(cPtr);
        auto icu = dynamic_cast<ICU *>(cPtr->getComp());
        if (icu)
        {
            std::string name = "ICU:";
            icu->setName(name += std::to_string(i));
            icu->setWakeCallback(std::bind(&CoKernel::wakeUpReady, this, std::placeholders::_1, std::string("icuwake")));
            icus_.push_back(icu);
        }
        
    }
    for (uint i = 0;i < cpuNum_;i++)
    {
        auto cPtr = std::make_shared<CompStarter>(CompStarter::COMP_CPU);
        comps_.push_back(cPtr);
        auto cpu = dynamic_cast<CPU *>(cPtr->getComp());
        if (cpu)
        {
            std::string name = "CPU:";
            cpu->setName(name += std::to_string(i));
            cpu->setScheCB(std::bind(&CoKernel::schedule, this));
            cpus_.push_back(cpu);
        }
    }
    
    icu_ = std::make_shared<ICU>();
    icu_->setName(std::string("ICU:") + std::to_string(icuNum_ -1));
    icu_->setWakeCallback(std::bind(&CoKernel::wakeUpReady, this, std::placeholders::_1, std::string("icuwake")));
    icus_.push_back(icu_.get());
}

void CoKernel::start()
{
    for (auto comp : comps_)
    {
        comp->run();
    }
    icu_->loop();
}

Lazy<int> CoKernel::waitFile(int fd, uint32_t events, WQCB cb, std::chrono::microseconds ti)
{
    int ret = 0;
    std::shared_ptr<FileWQ> fq;
    co_await CoRoLock(fMapLk_);
    auto file = fileWQPtrs_.find(fd);
    if (file != fileWQPtrs_.end())
    {
        fq = file->second;
    }
    co_await CoRoUnlock(fMapLk_);
    if (!fq)
    {
        ret = -1;
    }
    else
    {
        ret = co_await FileAwaiter{ fq ,events,std::move(cb),ti };
    }

    co_return ret;
}

Lazy<void> CoKernel::waitTime(const TimePoint &tp)
{
    co_await TimeAwaiter{ tp,icu_.get() };
}

Lazy<void> CoKernel::CoRoLock(MuCore &mu)
{
    co_await LockAwaiter{ &mu };
}

Lazy<void> CoKernel::CoRoUnlock(MuCore &mu)
{
    co_await UnlockAwaiter{ &mu};
}

Lazy<int> CoKernel::updateIRQ(int fd, uint32_t events)
{
    ICU *icu = nullptr;
    int ret = 0;
    uint i = 0;
    co_await CoRoLock(fMapLk_);
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    if (!found)
    {
        icu = icus_[0];
        auto fq = std::make_shared<FileWQ>(fd, icu);
        fq->setWEvents(events);
        ret = co_await ICUFAwaiter{ icu,[icu, &fq ,this]() -> int {
            std::cout << "do in " << Component::getCurComp()->getName() << std::endl;
        return icu->addIRQ(fq.get());
        }};
        if (!ret)
        {
            fileWQPtrs_.insert(FileWQMap::value_type{fd, fq});
            
            for (i=1;i < icuNum_;i++)
            {
                if (icus_[i]->irqn > icus_[0]->irqn)
                {
                    break;
                }
            }
            icus_[0]->irqn++;
            if (i != 1)
            {
                auto temp = icus_[0];
                icus_[0] = icus_[i - 1];
                icus_[i - 1] = temp;
                icus_[0]->pos = 0;
                icus_[i - 1]->pos = i - 1;
            }
        }
    }
    else
    {
        icu = static_cast<ICU *>(file->second->getICU());
        file->second->setWEvents(events);
        ret = co_await ICUFAwaiter{ icu,[icu, &fq = file->second,this]() -> int {
        return icu->modIRQ(fq.get());
        } };
        
    }
    co_await CoRoUnlock(fMapLk_);
    co_return ret;
}

Lazy<int> CoKernel::removeIRQ(int fd)
{
    ICU *icu = nullptr;
    int ret = 0;
    int i = 0;
    co_await CoRoLock(fMapLk_);
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    if (found)
    {
        icu = static_cast<ICU *>(file->second->getICU());
        ret = co_await ICUFAwaiter{ icu,[icu, &fq = file->second,this]() -> int {
        return icu->delIRQ(fq.get());
        } };
        fileWQPtrs_.erase(fd);
        for (i = icu->pos - 1;i >= 0;i--)
        {
            if (icus_[i]->irqn < icus_[icu->pos]->irqn)
            {
                break;
            }
        }
        icus_[icu->pos]->irqn--;
        if (i != icu->pos - 1)
        {
            icus_[icu->pos] = icus_[i + 1];
            icus_[i + 1] = icu;
            icus_[icu->pos]->pos = icu->pos;
            icu->pos = i + 1;
        }
    }
    co_await CoRoUnlock(fMapLk_);
    co_return ret;
}

void CoKernel::wakeUpReady(std::coroutine_handle<> &h,const std::string &str)
{
    //std::cout << str <<" wake up in " << Component::getCurComp()->getName() << std::endl;
    {
        std::lock_guard<std::mutex> lg(hmu);
        readyRo_.enqueue(h);
    }
    
    hcv.notify_one();
}

bool CoKernel::schedule()
{
    std::coroutine_handle<> h;
    std::unique_lock<std::mutex> ulo(hmu);
    hcv.wait(ulo, [&h,this]()->bool {return readyRo_.dequeue(h);});
    ulo.unlock();
    do
    {
        h.resume();
    } while (readyRo_.dequeue(h));
    return false;
}

void CoKernel::print_core(){
    std::cout << "do in " << Component::getCurComp()->getName() << std::endl;
}