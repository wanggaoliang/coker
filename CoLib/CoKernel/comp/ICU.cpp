#include "ICU.h"
#include "../../utils/ScopeExit.h"
#include <assert.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <fcntl.h>

const int kPollTimeMs = 1000000;

ICU::ICU()
    :Component(),
    poller_(new IRQPoll()),
    threadId_(std::this_thread::get_id()),
    irqn(0)
{
    int wakeupFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wakeupFd < 0)
    {
    }
    wakeUpWQ_.reset(new FileWQ(wakeupFd, this));
    wakeUpWQ_->setFDCallback(std::function([](int fd) {
        uint64_t tmp;
        read(fd, &tmp, sizeof(tmp));}));
    wakeUpWQ_->setWEvents(EPOLLIN | EPOLLET);
    poller_->updateIRQ(EPOLL_CTL_ADD, wakeUpWQ_.get());
    timerWQ_.reset(new TimeWQ(this));
    timerWQ_->setWEvents(EPOLLIN | EPOLLET);
    poller_->updateIRQ(EPOLL_CTL_ADD, timerWQ_.get());
}


ICU::~ICU()
{
    quit();

    while (looping_.load(std::memory_order_acquire))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    ::close(wakeUpWQ_->getFd());
}

void ICU::loop()
{
    IRQPtrList list;
    assert(!looping_);
    assertInLoopThread();
    looping_.store(true, std::memory_order_release);
    quit_.store(false, std::memory_order_release);

    auto loopFlagCleaner = makeScopeExit(
        [this]() { looping_.store(false, std::memory_order_release); });
    while (!quit_.load(std::memory_order_acquire))
    {
        XCompFunc func;
        while (funcs_.dequeue(func))
        {
            func();
        }
        for (auto i : list)
        {
            auto ge = i->wakeup();
            while (bool(ge))
            {
                auto h = ge();
                wakeCB_(h);
            }
        }
        list.clear();
        poller_->waitIRQ(list, kPollTimeMs);
    }
}

void ICU::quit()
{
    quit_.store(true, std::memory_order_release);

    if (!isInLoopThread())
    {
        wakeUp();
    }
}

/* 跨线程 eventfd read/write 线程安全*/
void ICU::wakeUp()
{
    uint64_t tmp = 1;
    write(wakeUpWQ_->getFd(), &tmp, sizeof(tmp));
}

void ICU::assertInLoopThread()
{
    if (!isInLoopThread())
    {
        exit(1);
    }
}
