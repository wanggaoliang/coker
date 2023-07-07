#pragma once
#include <type_traits>
#include <atomic>
#include <string>
#include "../../utils/NonCopyable.h"

class Component :NonCopyable
{
public:
    Component();
    ~Component();
    virtual void loop() = 0;

    virtual void quit() = 0;

    void setName(char *name)
    {
        name_ = name;
    }

    template<class T>
     requires std::is_convertible_v<T,std::string>
    void setName(T &&name)
    {
        name_ = name;
    }

    std::string getName() const
    {
        return name_;
    }

    static Component *getCurComp()
    {
        return curComp;
    }

protected:
    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    std::string name_;
    static thread_local Component *curComp;
};