#include "CPU.h"
#include "../../utils/ScopeExit.h"
#include <assert.h>
#include <signal.h>
#include <fcntl.h>

CPU::CPU()
    :Component(),
    threadId_(std::this_thread::get_id())
{
}


CPU::~CPU()
{
    quit();

    while (looping_.load(std::memory_order_acquire))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void CPU::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_.store(true, std::memory_order_release);
    quit_.store(false, std::memory_order_release);

    auto loopFlagCleaner = makeScopeExit(
        [this]() { looping_.store(false, std::memory_order_release); });
    while (!quit_.load(std::memory_order_acquire))
    {
        if (scheCB_)
        {
            scheCB_();
        }
        else
        {
            
        }
    }
}

void CPU::quit()
{
    quit_.store(true, std::memory_order_release);

    if (!isInLoopThread())
    {
    }
}

void CPU::assertInLoopThread()
{
    if (!isInLoopThread())
    {
        exit(1);
    }
}
