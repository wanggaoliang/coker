#include "CoLib/CoRo/Lazy.h"
#include "CoLib/CoRo/Task.h"
#include "CoLib/CoKernel/CoKernel.h"
#include "CoLib/ATime.h"
#include "CoLib/AMutex.h"
coasync::AMutex amu;

Task test1(int a)
{
    for (int i = 0; i < 10; i++)
    {
        co_await amu.lock();
        std::cout << 1 << ":" << a << ":" << i << std::endl;
        co_await coasync::sleep_for(std::chrono::seconds(1));
        co_await amu.unlock();
    }
    co_return;
}

Task init()
{
    for (int i = 0; i < 10; i++)
    {
        test1(i);
    }
    co_return;
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