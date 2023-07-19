#pragma once
#include <exception>
#include <concepts>
#include <coroutine>
#include "../utils/NonCopyable.h"

class ExhaustedException : std::exception {};
template <typename T>
class Generator :public NonCopyable
{
public:

    struct promise_type
    {
        std::suspend_always initial_suspend() noexcept { return {}; }

        std::suspend_always final_suspend() noexcept { return {}; }

        Generator get_return_object() noexcept
        {
            return Generator<T>(Generator<T>::Handle::from_promise(*this));
        }

        template<class U>
            requires std::is_convertible_v<U, T>
        std::suspend_always yield_value(U &&value)
        {
            this->value = std::forward<U>(value);
            ready = true;
            return {};
        }

        void unhandled_exception() noexcept
        {}

        T value;
        bool ready;
    };
    using Handle = std::coroutine_handle<promise_type>;
    using ValueType = T;
    
    explicit Generator(Handle h) : h_(h) {};

    Generator(Generator &&other) : h_(std::move(other.h_))
    {
        other.h_ = nullptr;
    }

    ~Generator()
    {
        if (h_)
        {
            h_.destroy();
            h_ = nullptr;
        }
    };

    Generator &operator =(Generator &&other)
    {
        h_ = std::move(other.h_);
        other.h_ = nullptr;
    }

    ValueType operator ()()
    {
        if (has_next())
        {
            h_.promise().ready = false;
            return h_.promise().value;
        }
        throw ExhaustedException();
    }

    explicit operator bool()
    {
        return has_next();
    }

protected:
    bool has_next()
    {
        if (h_.promise().ready)
        {
            return true;
        }
        else if (h_ && !h_.done())
        {
            h_.resume();
            if (h_.promise().ready)
            {
                return true;
            }
        }
        return false;
    }
    Handle h_;
};
