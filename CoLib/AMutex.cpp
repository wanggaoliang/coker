#include "CoKernel.h"
#include "AMutex.h"

Lazy<void> colib::AMutex::lock()
{
    co_await LockAwaiter{&mc};
}

Lazy<void> colib::AMutex::try_to_lock()
{
    co_await TryLockAwaiter{ &mc };
}

Lazy<void> colib::AMutex::try_lock_for()
{
    co_await TryLockAwaiter{ &mc };
}

Lazy<void> colib::AMutex::unlock()
{
    co_await UnlockAwaiter{ &mc };;
}