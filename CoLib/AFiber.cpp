#include "CoKernel.h"
#include "CoRo/Task.h"
#include "AFiber.h"

int TaskPromise::cnt = 1;

Task make_task(Lazy<void> lazy)
{
    co_await lazy;
}

void coasync::fiber::__run(Lazy<void> &&t)
{
    make_task(std::move(t));
    if (t._coro)
    {
        std::cout << "h" << std::endl;
    }
    else
    {
        std::cout << "!h" << std::endl;
    }
}

Lazy<void> coasync::__sleep_for(const std::chrono::microseconds &ti)
{
    co_await CoKernel::getKernel()->waitTime(std::chrono::steady_clock::now() + ti);
}

Lazy<void> coasync::sleep_util(const std::chrono::steady_clock::time_point &tp)
{
    co_await CoKernel::getKernel()->waitTime(tp);
}