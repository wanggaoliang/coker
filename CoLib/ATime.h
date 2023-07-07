#pragma once
#include <chrono>
#include "CoRo/Lazy.h"
namespace coasync
{
    Lazy<void> __sleep_for(const std::chrono::microseconds&);

    template<typename _Rep, typename _Period>
    inline Lazy<void> sleep_for(const std::chrono::duration<_Rep, _Period> &__rtime)
    {
        if (__rtime > __rtime.zero())
            co_await __sleep_for(std::chrono::duration_cast<std::chrono::microseconds>(__rtime));
        co_return;
    }


    Lazy<void> sleep_util(const std::chrono::steady_clock::time_point &);

};