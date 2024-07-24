#include "CoLib/CoRo/Lazy.h"
#include "CoLib/CoKernel/CoKernel.h"
#include "CoLib/AFiber.h"
#include "CoLib/AMutex.h"
colib::AMutex amu;

Lazy<void> test1(int a)
{
    for (int i = 0; i < 10; i++)
    {
        co_await amu.lock();
        std::cout << 1 << ":" << a << ":" << i << std::endl;
        co_await colib::sleep_for(std::chrono::seconds(1));
        co_await amu.unlock();
    }
    co_return;
}

void init()
{
    for (int i = 0; i < 10; i++)
    {
        colib::fiber(test1, i);
    }
}
int main()
{
    CoKernel::INIT(2, 1);
    init();
    std::cout << std::thread::hardware_concurrency() << std::endl;
    CoKernel::getKernel()->start();
    std::cout << "2" << std::endl;
    return 0;
}