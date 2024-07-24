#pragma once
#include <chrono>
#include "CoRo/Lazy.h"
namespace colib
{
    class fiber :NonCopyable
    {
    public:
        template<typename _Callable, typename... _Args>
            requires std::is_invocable_v<typename std::decay<_Callable>::type, typename std::decay<_Args>::type...>
        fiber(_Callable &&__f, _Args&&... __args)
        {
            __run(make_tmpLazy(std::forward<_Callable>(__f), std::forward<_Args>(__args)...));
        }

        fiber(fiber &&other) {}
        ~fiber()
        {
            if (0&& joinable())
                std::terminate();
        }

        Lazy<void> join();

        Lazy<void> detach();

        bool joinable()
        {
            if (!__M_fiber)
            {
                return true;
            }
            return false;
        };
    private:
        unsigned int __M_fiber;
    
        void __run(Lazy<void> &&);
    
        template<typename _Callable, typename... _Args>
            requires std::is_invocable_v<typename std::decay<_Callable>::type, typename std::decay<_Args>::type...>
        static Lazy<void> make_tmpLazy(_Callable __f, _Args... __args)
        {
            using ret = decltype(std::declval<_Callable>()(__args...));
            if constexpr (HasCoAwaitMethod<ret>)
            {
                co_await __f(__args...);
            }
            else
            {
                __f(__args...);
                co_return;
            }
        }

    };

    Lazy<void> sleep_util(const std::chrono::steady_clock::time_point &);

    template<typename _Rep, typename _Period>
    inline Lazy<void> sleep_for(const std::chrono::duration<_Rep, _Period> &__rtime)
    {
        if (__rtime > __rtime.zero())
            co_await sleep_util(std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::microseconds>(__rtime));
        co_return;
    }
};