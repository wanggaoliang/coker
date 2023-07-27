#include "IRQPoll.h"
#include <unistd.h>
#include <errno.h>

IRQPoll::IRQPoll()
{
    epoll_fd_ = epoll_create1(0);
}

IRQPoll::~IRQPoll()
{
    ::close(epoll_fd_);
}

int IRQPoll::updateIRQ(int op, IRQAbs *WQA)
{
    epoll_event event;
    event.data.ptr = WQA;
    event.events = WQA->getWEvents();
    auto ret = ::epoll_ctl(epoll_fd_, op, WQA->getFd(), &event);
    if (ret < 0)
    {
        ret = -errno;
    }
    return ret;
}

int IRQPoll::waitIRQ(IRQPtrList& list, int timeout)
{
    epoll_event events_[128];
    int num_events = epoll_wait(epoll_fd_, events_, 128, timeout);
    if (num_events < 0)
    {
        return num_events;
    }
    for (int i = 0;i < num_events;i++)
    {
        IRQAbs *WQA = static_cast<IRQAbs *>(events_[i].data.ptr);
        WQA->addREvents(events_[i].events);
        WQA->setWaked(true);
        list.emplace_back(WQA);
    }
    return num_events;
}
