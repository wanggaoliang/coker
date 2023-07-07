#include "CompStarter.h"
CompStarter::CompStarter(COMP_CATE cate)
    :cate_(cate),thread_([this]() { loopFuncs(); })
{
    auto f = promiseForCompPtr_.get_future();
    comp_ = f.get();
}

CompStarter::~CompStarter()
{
    run();
    std::shared_ptr<Component> comp;
    {
        std::unique_lock<std::mutex> lk(loopMutex_);
        comp = comp_;
    }
    if (comp)
    {
        comp->quit();
    }
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void CompStarter::wait()
{
    thread_.join();
}

void CompStarter::loopFuncs()
{

    thread_local static std::shared_ptr<Component> comp;
    switch (cate_)
    {
    case COMP_ICU:
        auto icu = new ICU();
        comp = std::shared_ptr<Component>(icu);
        break;
    case COMP_CPU:
        auto cpu = new CPU();
        comp = std::shared_ptr<Component>(cpu);
        break;
    default:
        break;

    }
    promiseForCompPtr_.set_value(comp);
    auto f = promiseForRun_.get_future();
    (void) f.get();
    comp->loop();
    {
        std::unique_lock<std::mutex> lk(loopMutex_);
        comp_ = nullptr;
    }
}

void CompStarter::run()
{
    std::call_once(once_, [this]() {
        promiseForRun_.set_value(1);});
}
