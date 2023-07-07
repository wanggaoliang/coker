#pragma once 
#include "Lazy.h"
#include "AMutex.h"

namespace coasync
{
    class ACond_variable
    {
    public:
        Lazy<void> wait();
        Lazy<void> notify_all();
        Lazy<void> notify_once();
    }
};