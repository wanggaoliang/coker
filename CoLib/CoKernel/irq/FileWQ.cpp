#include "FileWQ.h"

FileWQ::~FileWQ()
{
    if (fdCallbck_)
    {
        fdCallbck_(fd_);
    }
    for (auto iter = items_.begin(); iter != items_.end(); )
    {
        if (iter->cb_)
        {
            iter->cb_(fd_, revents_);
        }
        wakeCB_(iter->h_);
        iter = items_.erase(iter);
    }
}

void FileWQ::wakeup()
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
            if (wakeCB_)
            {
                wakeCB_(iter->h_);
            }

            iter = items_.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}