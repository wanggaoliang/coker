#include "CoKernel.h"
#include <iostream>

std::shared_ptr<CoKernel> CoKernel::kernel = nullptr;

CoKernel::CoKernel(uint num) :coreNum_(num)
{
    for (int i = 0;i < coreNum_ -1;i++)
    {
        auto cPtr = std::make_shared<ThreadCore>();
        thCores_.push_back(cPtr);
        irqs_.push_back(cPtr->getCore());
    }
    core_ = std::make_shared<Core>();
    irqs_.push_back(core_.get());
    for (auto irq : irqs_)
    {
        if (irq)
        {
            irq->setScheCB(std::bind(&CoKernel::schedule, this));
            irq->setTimeWakeCB(std::bind(&CoKernel::wakeUpReady, this, std::placeholders::_1));
        }
    }
    core_->bindCore2Thread();
}

void CoKernel::start()
{
    for (auto th : thCores_)
    {
        th->run();
    }
    core_->loop();
}

Lazy<int> CoKernel::waitFile(int fd, uint32_t events, WQCB cb)
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
        ret = co_await FileAwaiter{ fq ,events,std::move(cb) };
    }

    co_return ret;
}

Lazy<void> CoKernel::waitTime(const TimePoint &tp)
{
    co_await TimeAwaiter{ tp };
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
    Core *core = nullptr;
    int ret = 0;
    int i = 0;
    co_await CoRoLock(fMapLk_);
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    if (!found)
    {
        core = irqs_[0];
        auto fq = std::make_shared<FileWQ>(fd, core);
        fq->setWEvents(events);
        fq->setWakeCallback(std::bind(&CoKernel::wakeUpReady, this, std::placeholders::_1));
        ret = co_await XCoreFAwaiter{ core,[core, &fq ,this]() -> int {
            std::cout << "do in " << Core::getCurCore()->getIndex() << std::endl;
        return core->addIRQ(fq.get());
        }};
        if (!ret)
        {
            fileWQPtrs_.insert(FileWQMap::value_type{fd, fq});
            
            for (i=1;i < coreNum_;i++)
            {
                if (irqs_[i]->irqn > irqs_[0]->irqn)
                {
                    break;
                }
            }
            irqs_[0]->irqn++;
            if (i != 1)
            {
                auto temp = irqs_[0];
                irqs_[0] = irqs_[i - 1];
                irqs_[i - 1] = temp;
                irqs_[0]->pos = 0;
                irqs_[i - 1]->pos = i - 1;
            }
        }
    }
    else
    {
        core = static_cast<Core *>(file->second->getCore());
        file->second->setWEvents(events);
        ret = co_await XCoreFAwaiter{ core,[core, &fq = file->second,this]() -> int {
        return core->modIRQ(fq.get());
        } };
        
    }
    co_await CoRoUnlock(fMapLk_);
    co_return ret;
}

Lazy<int> CoKernel::removeIRQ(int fd)
{
    Core *core = nullptr;
    int ret = 0;
    int i = 0;
    co_await CoRoLock(fMapLk_);
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    if (found)
    {
        core = static_cast<Core *>(file->second->getCore());
        ret = co_await XCoreFAwaiter{ core,[core, &fq = file->second,this]() -> int {
        return core->delIRQ(fq.get());
        } };
        fileWQPtrs_.erase(fd);
        for (i = core->pos - 1;i >= 0;i--)
        {
            if (irqs_[i]->irqn < irqs_[core->pos]->irqn)
            {
                break;
            }
        }
        irqs_[core->pos]->irqn--;
        if (i != core->pos - 1)
        {
            irqs_[core->pos] = irqs_[i + 1];
            irqs_[i + 1] = core;
            irqs_[core->pos]->pos = core->pos;
            core->pos = i + 1;
        }
    }
    co_await CoRoUnlock(fMapLk_);
    co_return ret;
;
}

void CoKernel::wakeUpReady(std::coroutine_handle<> &h)
{
    readyRo_.enqueue(h);
}

bool CoKernel::schedule()
{
    std::coroutine_handle<> h;
    auto ret = readyRo_.dequeue(h);
    if(ret)
    {
        //std::cout << Core::getCurCore()->getIndex() << ":h.re" << std::endl;
        h.resume();
    }
    return ret;
}