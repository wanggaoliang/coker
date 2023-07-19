#include "FileWQ.h"

Generator<std::coroutine_handle<>> FileWQ::wakeup()
{
    if (fdCallbck_)
    {
        fdCallbck_(fd_);
    }

    for (auto iter = items_.begin(); iter != items_.end(); )
    {
        if (!revents_)
        {
            break;
        }
        else if (revents_ & iter->events_)
        {
            if (iter->cb_)
            {
                iter->cb_(fd_, revents_ & iter->events_);
            }
            revents_ &= ~(iter->events_);
            auto h = iter->h_;
            iter = items_.erase(iter);
            if (h)
            {
                co_yield h;
            }
        }
        else
        {
            iter++;
        }
    }
}