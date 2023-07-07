#pragma once
class  NonCopyable
{
  protected:
    NonCopyable()
    {
    }
    ~NonCopyable()
    {
    }
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
    // some uncopyable classes maybe support move constructor....
    NonCopyable(NonCopyable &&) noexcept(true) = default;
    NonCopyable &operator=(NonCopyable &&) noexcept(true) = default;
};

