#include "CoKernel.h"
#include "ATime.h"

Lazy<void> coasync::__sleep_for(const std::chrono::microseconds& ti)
{
    co_await CoKernel::getKernel()->waitTime(std::chrono::steady_clock::now() + ti);
}

Lazy<void> coasync::sleep_util(const std::chrono::steady_clock::time_point &tp)
{
    co_await CoKernel::getKernel()->waitTime(tp);
}