#include "CoKernel.h"
#include <iostream>

std::shared_ptr<CoKernel> CoKernel::kernel = nullptr;

CoKernel::CoKernel(uint icuNum, uint cpuNum) :icuNum_(icuNum), cpuNum_(cpuNum)
{
    for (int i = 0;i < icuNum_ - 1;i++)
    {
        auto cPtr = std::make_shared<CompStarter>(CompStarter::COMP_ICU);
        comps_.push_back(cPtr);
        auto icu = dynamic_cast<ICU *>(cPtr->getComp());
        if (icu)
        {
            std::string name = "ICU:";
            icu->setName(name += std::to_string(i));
            icus_.push_back(icu);
        }
        
    }
    for (int i = 0;i < cpuNum_;i++)
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
    icu_->setName(std::string("CPU")+std::to_string(icuNum_));
    timer_ = std::make_shared<TimeWQ>();
    timer_->setWEvents(EPOLLIN | EPOLLET);
    icu_->addIRQ(timer_.get());
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
    ICU *icu = nullptr;
    int ret = 0;
    int i = 0;
    co_await CoRoLock(fMapLk_);
    auto file = fileWQPtrs_.find(fd);
    auto found = file != fileWQPtrs_.end();
    if (!found)
    {
        icu = icus_[0];
        auto fq = std::make_shared<FileWQ>(fd, icu);
        fq->setWEvents(events);
        fq->setWakeCallback(std::bind(&CoKernel::wakeUpReady, this, std::placeholders::_1));
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