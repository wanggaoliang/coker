#include "CoKernel.h"
#include "CoRo/Task.h"
#include "AFiber.h"
#include "AFuture.h"

int TaskPromise::cnt = 1;

Task make_task(Lazy<void> lazy)
{
    co_await lazy;
}

Lazy<void> colib::fiber::join()
{
    co_return;
}

Lazy<void> colib::fiber::detach()
{
    co_return;
}

void colib::fiber::__run(Lazy<void> &&t)
{
    make_task(std::move(t));
}

Lazy<void> colib::sleep_util(const std::chrono::steady_clock::time_point &tp)
{
    co_await CoKernel::getKernel()->waitTime(tp);
}

void colib::async_run(Lazy<void> &&t)
{
    make_task(std::move(t));
}

void colib::waitflag::notify()
{
    std::lock_guard{ fl };
    flag = true;
    while (!waitQ.empty())
    {
        CoKernel::getKernel()->wakeUpReady(waitQ.front(), "test");
        waitQ.pop();
    }
}