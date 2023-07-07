#include "Component.h"

thread_local Component *Component::curComp = nullptr;

Component::Component():looping_(false),quit_(false)
{
    if (getCurComp())
    {
        
    }
    else
    {
        curComp = this;
    }
}