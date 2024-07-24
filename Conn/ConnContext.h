#pragma once
#include "Connection.h"
#include <memory>
class ConnContext :NonCopyable
{
public:
    ConnContext()
    {
    }
    ~ConnContext()= default;
    virtual Lazy<void> run() = 0;
};