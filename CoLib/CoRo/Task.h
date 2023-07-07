#include "../CoKernel/CoKernel.h"

class Task;

class TaskPromise
{
public:
    struct InitAwaiter
    {
        bool await_ready() const noexcept { return false; }

        auto await_suspend(std::coroutine_handle<> h) noexcept
        {
            CoKernel::getKernel()->wakeUpReady(h);
        }

        void await_resume() noexcept {}
    };
public:
    TaskPromise() : _tcb(cnt++,nullptr) {}

    InitAwaiter initial_suspend() noexcept { return {}; }

    std::suspend_never final_suspend() noexcept { return {}; }

    Task get_return_object() noexcept;
    void return_void() noexcept {}
    void unhandled_exception() noexcept
    {
    }
    template <typename Awaitable>
    auto await_transform(Awaitable &&awaitable)
    {
        if constexpr (HasCoAwaitMethod<Awaitable>)
        {
            return std::forward<Awaitable>(awaitable).coAwait();
        }
        else
        {
            return std::forward<Awaitable>(awaitable);
        }
    }

    UTCB _tcb;
    static int cnt;
};

int TaskPromise::cnt = 1;

class Task
{
public:
    using promise_type = TaskPromise;
    using Handle = std::coroutine_handle<promise_type>;
    Task(std::coroutine_handle<promise_type> coro) :h_(coro) {}
    ~Task() = default;
private:
    std::coroutine_handle<promise_type> h_;
};

inline Task TaskPromise::get_return_object() noexcept
{
    return Task(Task::Handle::from_promise(*this));
}