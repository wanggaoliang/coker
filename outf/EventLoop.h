#pragma once
#include "NonCopyable.h"
#include "EPoller.h"
#include "WaitQueue.h"
#include <memory>
#include <vector>
#include <mutex>
#include <map>
#include <functional>
#include <chrono>
#include <limits>
#include <atomic>

class Poller;
class TimerQueue;
class Channel;
using EventList = std::vector<Channel *>;
using Func = std::function<void()>;
using TimerId = uint64_t;
enum
{
    InvalidTimerId = 0
};

/**
 * @brief As the name implies, this class represents an event loop that runs in
 * a perticular thread. The event loop can handle network I/O events and timers
 * in asynchronous mode.
 * @note An event loop object always belongs to a separate thread, and there is
 * one event loop object at most in a thread. We can call an event loop object
 * the event loop of the thread it belongs to, or call that thread the thread of
 * the event loop.
 */
class EventLoop : NonCopyable
{
  public:
    EventLoop();
    ~EventLoop();

    /**
     * @brief Run the event loop. This method will be blocked until the event
     * loop exits.
     *
     */
    void run();

    /**
     * @brief Let the event loop quit.
     *
     */
    void quit();

    /**
     * @brief Update channel status. This method is usually used internally.
     *
     * @param chl
     */
    void updateChannel(Channel *chl);

    /**
     * @brief Remove a channel from the event loop. This method is usually used
     * internally.
     *
     * @param chl
     */
    void removeChannel(Channel *chl);

    /**
     * @brief Return the index of the event loop.
     *
     * @return size_t
     */
    size_t index()
    {
        return index_;
    }

    /**
     * @brief Set the index of the event loop.
     *
     * @param index
     */
    void setIndex(size_t index)
    {
        index_ = index;
    }

    /**
     * @brief Return true if the event loop is running.
     *
     * @return true
     * @return false
     */
    bool isRunning()
    {
        return looping_.load(std::memory_order_acquire) &&
               (!quit_.load(std::memory_order_acquire));
    }

    /**
     * @brief Check if the event loop is calling a function.
     *
     * @return true
     * @return false
     */
    bool isCallingFunctions()
    {
        return callingFuncs_;
    }

    /**
     * @brief Run functions when the event loop quits
     *
     * @param cb the function to run
     * @note the function runs on the thread that quits the EventLoop
     */
    void runOnQuit(Func &&cb);
    void runOnQuit(const Func &cb);

  private:
    std::atomic<bool> looping_;
    std::thread::id threadId_;
    std::atomic<bool> quit_;
    std::unique_ptr<Poller> poller_;

    WQAPtrList activeEvents_;
    bool callingFuncs_{ false };
    std::unique_ptr<TimerWaitQueue> timerWQ_;
    std::map<FileWaitQueue> FileWQs_;
    size_t index_{ std::numeric_limits<size_t>::max() };
};
