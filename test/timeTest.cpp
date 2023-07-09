#include "CoLib/CoRo/Lazy.h"
#include "CoLib/CoRo/Task.h"
#include "CoLib/CoKernel/CoKernel.h"
#include "CoLib/ATime.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <string.h>

Task test_thread_sleep(int in)
{
    auto old = std::chrono::steady_clock::now();
    for (int i = 0; i < 4;i++)
    {
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    auto interval = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - old);
    std::cout << in << ": end test:"<<interval.count() << std::endl;
    co_return;
}

Task test_coro_sleep(int in)
{
    auto old = std::chrono::steady_clock::now();
    for (int i = 0; i < 4;i++)
    {
        
        co_await coasync::sleep_for(std::chrono::seconds(3));
    }
    auto interval = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - old);
    std::cout << in << ": end test:"<<interval.count() << std::endl;
    co_return;
}

Task init()
{
    for (int i = 0; i < 5; i++)
    {
        test_thread_sleep(i);
    }
    co_return;
}

int main()
{
    CoKernel::INIT(2,1);
    init();
    std::cout << std::thread::hardware_concurrency() << std::endl;
    CoKernel::getKernel()->start();
    std::cout << "2" << std::endl;
    return 0;
}