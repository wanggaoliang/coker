#pragma once
#include <atomic>

class SpinLock
{
private:
    std::atomic_flag flag;

public:
    SpinLock() : flag(ATOMIC_FLAG_INIT) {}

    void lock()
    {
        while (flag.test_and_set(std::memory_order_acquire))
        {
            // 自旋等待锁释放
        }
    }

    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};