#include "Timer.h"
#include "Logger.h"

std::atomic<u_int64_t> Timer::s_numCreated_(0);

void Timer::run() const 
{ 
    if(callback_)
    {
        // LOG_ERROR("Timer::run() callback_");
        callback_();
    }
    else{
        LOG_ERROR("Timer::run() callback_ is empty...");
    }
}

void Timer::restart(Timestamp now)
{
    if(repeat_)
    {
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}