#pragma once

#include "noncopyable.h"
#include "EventLoop.h"
#include "Timestamp.h"
#include "Helper.h"
#include "Callbacks.h"
#include "Channel.h"

#include <set>
#include <vector>

class Timer;
class TimerId;
// class Channel;

class TimerQueue : noncopyable
{
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(TimerCallback cb, Timestamp when, double interval);
    void cancel(TimerId timerId);

private:
    using Entry =  std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;
    using ActiveTimer = std::pair<Timer*, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    void addTimerInLoop(Timer* timer);
    void canceInLoop(TimerId timerId);
    void handleRead();
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    EventLoop *loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    TimerList timers_;

    ActiveTimerSet activeTimers_;
    std::atomic_bool callingExpiredTimers_;
    ActiveTimerSet cancelingTimers_;
};