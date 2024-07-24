#pragma once
#include "../../utils/Common.h"
#include "IRQAbs.h"
#include <list>

struct ioret
{
    int ret;
    bool exhausted;
    bool block;
};

using WQCB = std::function<ioret(int, uint)>;
using WQCBWrap = std::function<ioret(int, uint)>;
using FDCB = std::function<void(int)>;
class FileWQ :public IRQAbs
{
public:
    struct WaitItem
    {
        std::coroutine_handle<> h_;
        WQCBWrap cb_;
        uint events_;
        WaitItem()
        {}
        
        WaitItem(const std::coroutine_handle<> &h, uint events, WQCBWrap &&cb)
            :h_(h), cb_(std::move(cb)), events_(events)
        {}
    };

    FileWQ(int fd, void *icu):IRQAbs(fd,icu) {}

    ~FileWQ(){};
    
    Generator<std::coroutine_handle<>> wakeup() override;

    
    template<typename T>
    requires std::is_convertible_v<T, WQCBWrap>
    std::list<WaitItem>::iterator addWait(const std::coroutine_handle<> &h, uint events, T &&cb)
    {
        return items_.emplace(items_.end(), h, events, std::forward<T>(cb));
    }

    std::list<WaitItem>::iterator addWait()
    {
        return items_.emplace(items_.end());
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

