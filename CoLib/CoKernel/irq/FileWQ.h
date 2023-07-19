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
    struct WaitItem
    {
        WQCBWrap cb_;
        std::coroutine_handle<> h_;
        uint events_;
        WaitItem()
        {}
        
        WaitItem(const std::coroutine_handle<> &h, uint events, WQCBWrap &&cb)
            :h_(h), events_(events),cb_(std::move(cb))
        {}
    };

    FileWQ(int fd, void *icu):IRQAbs(fd,icu) {}

    ~FileWQ(){};
    
    Generator<std::coroutine_handle<>> wakeup() override;

    
    template<typename T>
    requires std::is_convertible_v<T, WQCBWrap>
    std::list<WaitItem>::iterator addWait(const std::coroutine_handle<> &h, uint events, T &&cb)
    {
        items_.emplace_back(h, events, std::forward<T>(cb));
        return --items_.end();
    }

    std::list<WaitItem>::iterator addWait()
    {
        items_.emplace_back();
        return --items_.end();
    }

    template<typename T>
    requires std::is_convertible_v<T,FDCB>
    void setFDCallback(T &&func)
    {
        fdCallbck_ = std::forward<T>(func);
    }

    void delWait(std::list<WaitItem>::iterator it)
    {
        items_.erase(it);
    }

private:
    std::list<WaitItem> items_;
    FDCB fdCallbck_;
};

