#pragma once 
#include "Lazy.h"
#include "AMutex.h"
#include "condition_variable"
std::condition_variable cv;
namespace colib
{
    class ACond_variable
    {
    public:
        Lazy<void> wait(AMutex &__mu);

        template<typename _Predicate>
            requires std::is_invocable_v<typename std::decay<_Predicate>::type>
        Lazy<void> wait(AMutex &__mu, _Predicate __p)
        {
            while (!__p())
                co_await wait(__mu);
        }

        Lazy<void> wait_until();
        Lazy<void> notify_all();
        Lazy<void> notify_once();
    private:
        
    };
};