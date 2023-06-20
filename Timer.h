#pragma once

#include "Timestamp.h"
#include "noncopyable.h"
#include "Callbacks.h"

#include <atomic>

class Timer : noncopyable
{
public:   // 回调函数 时间戳 循环间隔 是否重复
    Timer(const TimerCallback& cb, Timestamp when, double interval)
        : callback_(cb)
        , expiration_(when)
        , interval_(interval)
        , repeat_(interval > 0.0)
        , sequence_(++s_numCreated_)
    {
    }

    void run() const;
    bool repeat() const { return repeat_; }
    Timestamp expiration() { return expiration_; }
    int64_t sequence() const { return sequence_; }
    void restart(Timestamp now);
    static int64_t numCreated() { return s_numCreated_; }

private:
    const TimerCallback callback_;
    Timestamp expiration_;  //时间戳
    const double interval_; //间隔
    const bool repeat_; //是否重复
    const int64_t sequence_;//序列号，即唯一ID

    static std::atomic<u_int64_t> s_numCreated_;//原子计数
};
