#include "Lazy.h"
#include "Task.h"
#include <random>
#include <chrono>
#include "utils/MpmcQueue.h"
#include <thread>
#include <iostream>
#include <future>

MpmcQueue<int> q1{};
std::atomic<int> in=0;
std::atomic<int> out=0;

void pro()
{
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<>dis(1, 2);
    std::uniform_int_distribution<>num(5000, 5001);
    
    for (int i = 0;i < 1;i++)
    {
        int a;
        auto sum = num(gen);
        for (int j = 0;j < sum;j++)
        {
            auto t = dis(gen);
            q1.enqueue(t);
            in.fetch_add(t);
        }
        while (q1.dequeue(a))
        {
            out.fetch_add(a);
        }

    }

}

void test1()
{
    int a;
    std::vector<int> m{7, 3, 2, 5, 1, 9, 2, 5};
    for (auto j : m)
    {
        for (int i = 0;i < j;i++)
        {
            q1.enqueue(i);
        }
        std::cout << "insert over"<< std::endl;
        while (q1.dequeue(a))
        {
            std::cout << "get :" << a << std::endl;
        }
    }
}

void test2()
{
    int a;
    for (auto j=0;j<10;j++)
    {
        q1.enqueue(j);

        while (q1.dequeue(a))
        {
            std::cout << "get :" << a << std::endl;
        }
    }
}

void test3()
{
    int a;
    for (auto j = 0;j < 10;j++)
    {
        q1.enqueue(j);
    }
    while (q1.dequeue(a))
    {
        std::cout << "get :" << a << std::endl;
    }
}


void test4()
{
    int a = 0;
    for (int i = 0;i < 100;i++)
    {
        q1.enqueue(i);
    }
    auto th = std::thread([] {
        int a = 0;
        for (int i = 200;i < 400;i++)
        {
            q1.enqueue(i);
        }
        while (q1.dequeue(a))
        {
            std::cout << "th get :" << a << std::endl;
        }
                          });
    th.detach();
    while (q1.dequeue(a))
    {
        std::cout << "get :" << a << std::endl;
    }

}

void test6(int index)
{
    int a;
    std::vector<std::thread> ths;
    for (int k = 0;k < 10;k++)
    {
        ths.emplace_back(pro);
    }
    for (auto &th : ths)
    {
        th.join();
    }
    std::cout << "in :" << in << "out:" << out << std::endl;
    while (q1.dequeue(a))
    {
        std::cout << "main get" << a << std::endl;
    }
    std::cout << "main end:" << index <<" m:"<<q1.m<< std::endl;
}

int main()
{
    for (int i = 0;i < 20;i++)
    {
        test6(i);
    }
    return 0;
}