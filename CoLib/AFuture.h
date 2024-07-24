#pragma once

#pragma once
#include <chrono>
#include "CoRo/Lazy.h"
#include <future>
#include <memory>
#include <queue>
#include <mutex>
namespace colib
{
    template <class T>
    struct lazy_result
    {
        using type = T;
    };

    template <class T>
    struct lazy_result<Lazy<T>>
    {
        using type = T;
    };
    
    template <typename _Callable, typename... _Args>
    using async_result_t = typename lazy_result<std::invoke_result_t<_Callable, _Args...>>::type;


    class waitflag :NonCopyable
    {
    public:
        waitflag() :flag(false) {}
        ~waitflag() = default;
        bool wait(std::coroutine_handle<> h)
        {
            std::lock_guard{ fl };
            if (!flag)
            {
                waitQ.push(h);
            }
            return flag;
        }
        void notify();
    private:
        std::mutex fl;
        bool flag;
        std::queue<std::coroutine_handle<>> waitQ;
    };

    struct SetAwaiter
    {
        waitflag *wf_ptr;
        SetAwaiter(waitflag *ptr) :wf_ptr(ptr)
        {
        };

        ~SetAwaiter() = default;

        bool await_ready() const noexcept
        {
            return false;
        }

        bool await_suspend(std::coroutine_handle<> handle)
        {
            wf_ptr->notify();
            return false;
        }

        void await_resume() noexcept
        {}
    };

    struct GetAwaiter
    {
        waitflag* wf_ptr;
        GetAwaiter(waitflag* ptr) :wf_ptr(ptr)
        {
        };

        ~GetAwaiter() = default;

        bool await_ready() const noexcept
        {
            return false;
        }

        bool await_suspend(std::coroutine_handle<> handle)
        {
            return !wf_ptr->wait(handle);
        }

        void await_resume() noexcept
        {}
    };

    template<class T>
    struct state : NonCopyable
    {
        waitflag wf;
        T value;
    };

    template<>
    struct state<void> : NonCopyable
    {
        waitflag wf;
    };

    template<class T>
    class future
    {
    public:
        future() = default;

        future(std::shared_ptr<state<T>> ptr) :_state_ptr(ptr)
        {
        }

        ~future() = default;

        Lazy<T> get()
        {
            co_await GetAwaiter{ &_state_ptr->wf };
            if constexpr (!std::is_void_v<T>)
            {
                co_return _state_ptr->value;
            }
        }

    private:
        std::shared_ptr<state<T>> _state_ptr;
    };

    template<class T>
    class promise : NonCopyable
    {
    public:
        promise() :_state_ptr(new state<T>()) {}
        ~promise(){}
        void set_value(T &arg)
        {
            if constexpr (!std::is_void_v<T>)
            {
                _state_ptr->value = arg;
            }
            co_await SetAwaiter{ &_state_ptr->wf };
        }
        void set_value(T &&arg)
        {
            if constexpr (!std::is_void_v<T>)
            {
                _state_ptr->value = std::move(arg);
            }
            co_await SetAwaiter{ &_state_ptr->wf };
        }
        future<T> get_future()
        {
            return future{ _state_ptr };
        }
    private:
        std::shared_ptr<state<T>> _state_ptr;
    };

    void async_run(Lazy<void> &&);

    template<typename _Callable, typename... _Args, typename CBRET = std::invoke_result_t<_Callable, _Args...>, typename RET = lazy_result<CBRET>::type>
    Lazy<void> async_help(std::shared_ptr<state<RET>> _state,_Callable __f, _Args... __args)
    {
        if constexpr (HasCoAwaitMethod<CBRET> && !std::is_void_v<RET>)
        {
            _state->value = co_await __f(__args...);
            co_await SetAwaiter{ &_state->wf };
        }
        else if constexpr (HasCoAwaitMethod<CBRET> && std::is_void_v<RET>)
        {
            co_await __f(__args...);
            co_await SetAwaiter{ &_state->wf };
        }
        else if constexpr (!HasCoAwaitMethod<CBRET> && std::is_void_v<RET>)
        {
            __f(__args...);
            co_await SetAwaiter{ &_state->wf };
        }
        else
        {
            _state->value = __f(__args...);
            co_await SetAwaiter{ &_state->wf };
        }
    }

    template<typename _Callable, typename... _Args, typename RET = async_result_t<_Callable, _Args...>>
    future<RET> async(_Callable &&__f, _Args&&... __args)
    {
        std::shared_ptr<state<RET>> _state = std::make_shared<state<RET>>();
        async_run(async_help(_state, std::forward<_Callable>(__f), std::forward<_Args>(__args)...));
        return future{ _state };
    }
};