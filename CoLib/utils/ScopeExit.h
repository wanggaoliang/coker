#pragma once
template <typename F>
struct ScopeExit
{
    ScopeExit(F &&f): f_(std::move(f))
    {}
    ScopeExit(const F& f): f_(f)
    {}
    ~ScopeExit()
    {
        f_();
    }
    F f_;
};

template <typename F>
ScopeExit<F> makeScopeExit(F &&f)
{
    return ScopeExit<F>(std::forward<F>(f));
};