#include "CoKernel.h"
#include "AMutex.h"

Lazy<void> coasync::AMutex::lock()
{
    co_await LockAwaiter{&mc};
}

Lazy<void> coasync::AMutex::try_to_lock()
{
    co_await TryLockAwaiter{ &mc };
}

Lazy<void> coasync::AMutex::unlock()
{
    co_await UnlockAwaiter{ &mc };;
}