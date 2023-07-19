#pragma once
#include <sys/epoll.h>
#include "../irq/IRQAbs.h"

class IRQPoll
{
public:
    IRQPoll();
    ~IRQPoll();

    int updateIRQ(int, IRQAbs*);
    int waitIRQ(IRQPtrList&,int);

private:
    int epoll_fd_;
};