#pragma once
#include <atomic>
#include <concepts>
#include <memory>
#include "SpinLock.h"
template <typename T>
class MpmcQueue
{
private:
    struct BufferNode
    {
        BufferNode() = default;
        BufferNode(const T &data) : data(data)
        {}
        BufferNode(T &&data) : data(std::move(data))
        {}
        T data;
        std::atomic<BufferNode *> next_{nullptr};
    };
public:
    MpmcQueue()
        : sHead_(new BufferNode), sTail_(sHead_.load(std::memory_order_relaxed)),
        uHead_(new BufferNode), uTail_(uHead_.load(std::memory_order_relaxed))
    {
    }
    ~MpmcQueue()
    {
        BufferNode *temp = nullptr;
        while ((temp = sHead_.load(std::memory_order_acquire)) != nullptr)
        {
            sHead_ = temp->next_.load(std::memory_order_acquire);
            delete temp;
        }
        while ((temp = uHead_.load(std::memory_order_acquire)) != nullptr)
        {
            uHead_ = temp->next_.load(std::memory_order_acquire);
            delete temp;
        }
    }

    template<typename U>
    requires std::is_convertible_v<U,T>
    void enqueue(U &&input)
    {
        BufferNode *pres = nullptr;
        BufferNode *nexts = nullptr;
        {
            std::lock_guard sgLk(sLk_);
            pres = sHead_.load(std::memory_order_relaxed);
            while (!(nexts = pres->next_.load(std::memory_order_acquire)))
            {
                if (sHead_ == sTail_)
                {
                    nexts = new BufferNode{};
                    sTail_ = nexts;
                    break;
                }
            }
            sHead_ = nexts;  
        }
        pres->data = std::forward<U>(input);
        pres->next_ = nullptr;
        auto preu = uTail_.exchange(pres, std::memory_order_acq_rel);
        preu->next_.store(pres, std::memory_order_release);
    }

    bool dequeue(T &output)
    {
        BufferNode *preu = nullptr;
        BufferNode *nextu = nullptr;
        {
            std::lock_guard ugLk(uLk_);
            preu = uHead_.load(std::memory_order_relaxed);
            while (!(nextu = preu->next_.load(std::memory_order_acquire)))
            {
                if (uHead_ == uTail_)
                {
                    return false;
                }
            }
            uHead_ = nextu;
            output = std::move(nextu->data);
        }
        preu->next_ = nullptr;
        auto pres = sTail_.exchange(preu, std::memory_order_acq_rel);
        pres->next_.store(preu, std::memory_order_release);
        return true;
    }
private:

    std::atomic<BufferNode *> uHead_;
    std::atomic<BufferNode *> uTail_;
    SpinLock uLk_;
        
    std::atomic<BufferNode *> sHead_;
    std::atomic<BufferNode *> sTail_;
    SpinLock sLk_;
};
