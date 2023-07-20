#pragma once
#include <chrono>
#include "CoRo/Lazy.h"
namespace coasync
{
    class fiber:NonCopyable
    {
    public:
        explicit fiber() noexcept
        {
            std::cout << "default fiber" << std::endl;
        };

        template<typename _Callable, typename... _Args>
            requires std::is_invocable_v<typename std::decay<_Callable>::type, typename std::decay<_Args>::type...>
        fiber(_Callable &&__f, _Args&&... __args)
        {
            std::cout << "fiber" << std::endl;
            __run(make_tmpLazy(std::forward<_Callable>(__f), std::forward<_Args>(__args)...));

        }
        fiber(fiber&& other){}
        ~fiber() {}
    private:
        int id;
        void __run(Lazy<void> &&);
        template<typename _Callable, typename... _Args>
            requires std::is_invocable_v<typename std::decay<_Callable>::type, typename std::decay<_Args>::type...>
        static Lazy<void> make_tmpLazy(_Callable __f, _Args... __args)
        {
            using ret = decltype(std::declval<_Callable>()(__args...));
            //using cla = Template_Type_Traits<ret>::classType;
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
    
    Lazy<void> __sleep_for(const std::chrono::microseconds &);

    template<typename _Rep, typename _Period>
    inline Lazy<void> sleep_for(const std::chrono::duration<_Rep, _Period> &__rtime)
    {
        if (__rtime > __rtime.zero())
            co_await __sleep_for(std::chrono::duration_cast<std::chrono::microseconds>(__rtime));
        co_return;
    }


    Lazy<void> sleep_util(const std::chrono::steady_clock::time_point &);

};