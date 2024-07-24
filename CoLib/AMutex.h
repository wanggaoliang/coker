#include "CoRo/Lazy.h"
#include "MuCore.h"

namespace colib
{
    class AMutex
    {
    public:
        Lazy<void> lock();
        Lazy<void> try_to_lock();
        Lazy<void> try_lock_for();
        Lazy<void> unlock();
    private:
        MuCore mc;
    };
};