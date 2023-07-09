#pragma once
#include "../../utils/Common.h"
#include "Component.h"
#include <memory>
#include <vector>
#include <thread>
class CPU : public Component
{
public:

    CPU();
    ~CPU();

    void loop() override;

    void quit() override;

    bool isInLoopThread() const
    {
        return threadId_ == std::this_thread::get_id();
    };

    void assertInLoopThread();

    template<class T>
        requires std::is_convertible_v<T, ScheCB>
    void setScheCB(T &&cb)
    {
        scheCB_ = std::forward<T>(cb);
    }

private:
    std::thread::id threadId_;
    ScheCB scheCB_;
};