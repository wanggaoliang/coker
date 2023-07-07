#pragma once
#include "../../utils/Common.h"
#include "IRQAbs.h"
#include <list>

struct ioret
{
    int ret;
    bool block;
};

using WQCB = std::function<ioret(int, uint)>;
using WQCBWrap = std::function<void(int, uint)>;
using FDCB = std::function<void(int)>;
class FileWQ :public IRQAbs
{
public:
    struct waitItem
    {
        WQCBWrap cb_;
        std::coroutine_handle<> h_;
        uint events_;
        waitItem(const std::coroutine_handle<> &h, uint events, WQCBWrap&& cb)
            :h_(h), events_(events),cb_(std::move(cb))
        {}
    };

    FileWQ(int fd, void *icu):IRQAbs(fd,icu) {}

    ~FileWQ();
    
    void wakeup() override;

    
    template<typename T>
    requires std::is_convertible_v<T, WQCBWrap>
    void addWait(const std::coroutine_handle<> &h, uint events, T &&cb)
    {
        items_.emplace_back(h, events, std::forward<T>(cb));
    }

    template<typename T>
    requires std::is_convertible_v<T,WakeCB>
    void setWakeCallback(T &&func)
    {
        wakeCB_ = std::forward<T>(func);
    }

    template<typename T>
    requires std::is_convertible_v<T,FDCB>
    void setFDCallback(T &&func)
    {
        fdCallbck_ = std::forward<T>(func);
    }

private:
    std::list<waitItem> items_;
    WakeCB wakeCB_;
    FDCB fdCallbck_;
    
};

