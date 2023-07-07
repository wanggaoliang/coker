#include "Lazy.h"
#include "MuCore.h"

namespace coasync
{
    class AMutex
    {
    public:
        Lazy<void> lock();
        Lazy<void> try_to_lock();
        Lazy<void> unlock();
    private:
        MuCore mc;
    };
};