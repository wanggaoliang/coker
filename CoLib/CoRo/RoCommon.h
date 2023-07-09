#pragma once
#include <concepts>
#include <coroutine>
#include <memory>
#include <iostream>
#include <utility>
#include "../utils/Attribute.h"
#include "../utils/NonCopyable.h"

struct UTCB
{
    int tid;
    void *core;
    UTCB(int t, void *c) :tid(t), core(c) {}
};

template<typename Awaitable>
concept HasCoAwaitMethod = requires(Awaitable && awaitable)
{
    std::forward<Awaitable>(awaitable).coAwait();
};

template<typename PromiseType>
concept HasUTCBStar = requires(PromiseType a)
{
    {a._tcb}->std::same_as<UTCB *&>;
};

template<typename PromiseType>
concept HasUTCBStruct = requires(PromiseType a)
{
    {a._tcb}->std::same_as<UTCB &>;
};

template<typename PromiseType>
concept HasUTCB = HasUTCBStruct<PromiseType> || HasUTCBStar<PromiseType>;

struct GetTCB
{
    UTCB *tcb;
    GetTCB() :tcb(nullptr) {}

    bool await_ready() const noexcept { return false; }

    template <typename PromiseType>
        requires HasUTCB < PromiseType>
    auto await_suspend(std::coroutine_handle<PromiseType> h) noexcept
    {
        if constexpr (HasUTCBStar<PromiseType>)
        {
            tcb = h.promise()._tcb;
        }
        else
        {
            tcb = &h.promise()._tcb;
        }
        return false;
    }

    UTCB *await_resume() noexcept
    {
        return tcb;
    }
};
