#pragma once
#include "Component.h"
#include "ICU.h"
#include "CPU.h"
#include <mutex>
#include <thread>
#include <future>
#include <memory>
class CompStarter
{
public:
    
    enum COMP_CATE
    {
        COMP_ICU,
        COMP_CPU,
        COMP_INVALID
    };
    
    CompStarter(COMP_CATE);
    
    ~CompStarter();
    
    void run();

    void wait();

    Component *getComp() const
    {
        return comp_.get();
    }
    
    
private:
    void loopFuncs();
    std::shared_ptr<Component> comp_;
    
    std::once_flag once_;
    std::thread thread_;
    std::mutex loopMutex_;

    std::promise<std::shared_ptr<Component>> promiseForCompPtr_;
    std::promise<int> promiseForRun_;
    COMP_CATE cate_;
};

