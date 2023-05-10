#include "TimerQueue.h"
#include "Logger.h"
#include "Timer.h"
#include "TimerId.h"

#include <sys/timerfd.h>
#include <strings.h>

namespace detailTimer
{
    int createTimerfd()
    {
        int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if(timerfd < 0)
        {
            LOG_FATAL("failed in timerfd_create...");
        }
        return timerfd;
    }

    struct timespec howMuchTimeFromNow(Timestamp when)
    {
        int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
        if(microseconds < 100)
        {
            microseconds = 100;
        }
        struct timespec ts;
        ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<time_t>(microseconds%Timestamp::kMicroSecondsPerSecond)*1000;
        return ts;
    }

    void resetTimerfd(int timerfd, Timestamp expiration)
    {
        struct itimerspec newValue;
        struct itimerspec oldValue;
        bzero(&newValue, sizeof newValue);
        bzero(&oldValue, sizeof oldValue);

        newValue.it_value = detailTimer::howMuchTimeFromNow(expiration);
        int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
        if(ret)
        {
            LOG_ERROR("timerfd_settime(");
        }
    }

    void readTimerfd(int timerfd, Timestamp now)
    {
        uint64_t howmany;
        ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
        if(n != sizeof(howmany))
        {
            LOG_ERROR("TimerQueue::readTimerfd() reads %d bytes instead of 8", n);
        }
    }
}

TimerQueue::TimerQueue(EventLoop* loop)
            : loop_(loop)
            , timerfd_(detailTimer::createTimerfd())
            , timerfdChannel_(loop, timerfd_)
            , timers_()
            , callingExpiredTimers_(false)
{
    LOG_INFO("TimeQueue  timerfd = %d\n", timerfd_);
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for(const Entry& timer : timers_)
    {
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    Timer *timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(std::bind(&TimerQueue::canceInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if(earliestChanged)
    {
        detailTimer::resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::canceInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if(it != activeTimers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        delete it->first;
        activeTimers_.erase(it);
    }
    else if(callingExpiredTimers_)
    {
        cancelingTimers_.insert(timer);
    }
}

void TimerQueue::handleRead()
{
    // LOG_INFO("TimerQueue::handleRead : ");
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    detailTimer::readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();

    // LOG_INFO("TimerQueue::handleRead() expired.size = %d ", expired.size());
    for(const Entry& it : expired)
    {
        it.second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);//找到第一个大于等于参数的位置，返回迭代器
    std::copy(timers_.begin(), end, back_inserter(expired));//插入到末尾
    timers_.erase(timers_.begin(), end);

    for(const Entry& it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
    }
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpire;
    for(const Entry& it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        if(it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it.second->restart(now);
            insert(it.second);
        }
        else
        {
            delete it.second;
        }
    }
    if(!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }
    if(nextExpire.valid())
    {
        detailTimer::resetTimerfd(timerfd_, nextExpire);
    }
}

bool TimerQueue::insert(Timer* timer)
{
    loop_->assertInLoopThread();
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool> result\
        = timers_.insert(Entry(when, timer));
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result\
        = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    }
    return earliestChanged;
}
