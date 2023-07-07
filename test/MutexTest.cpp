#include "CoRo/Lazy.h"
#include "CoRo/Task.h"
#include "CoKernel/CoKernel.h"
#include <iostream>
#include <chrono>
#include "CoKernel/AMutex.h"
#include "CoKernel/ATime.h"
coasync::AMutex amu;

Task test1(int a)
{
    for (int i = 0;i < 10;i++)
    {
        co_await amu.lock();
        std::cout << 1 << ":" << a << ":" << i << std::endl;
        auto t = std::chrono::steady_clock::now() + std::chrono::microseconds(1000000);
        co_await coasync::sleep_for(std::chrono::seconds(7));
        co_await amu.unlock();
    }
    co_return;
}


Task test()
{
    for (int i = 0;i < 10;i++)
    {
        test1(i);
    }
    co_return;
}
int main()
{
    std::cout << "1" << std::endl;
    CoKernel::INIT(3);
    test();
    std::cout << "3" << std::endl;
    CoKernel::getKernel()->start();
    std::cout << "2" << std::endl;
    return 0;
}