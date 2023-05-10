#pragma once

#include "noncopyable.h"
#include "Thread.h"
#include "Logger.h"
#include "CurrentThread.h"
#include "Mutex.h"
#include "Condition.h"

#include <functional>
#include <condition_variable>
// #include <mutex>
#include <vector>
#include <deque>

class ThreadPool : noncopyable
{
public:
    using Task = std::function<void()>;
    explicit ThreadPool(const std::string& name = "ThreadPoll");
    ~ThreadPool();

    void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
    void setThreadInitCallback(const Task &cb) { threadInitCallback_ = cb; }

    void start(int numThreads);
    void stop();

    const std::string &name() const { return name_; }
    size_t queueSize() const;

    void run(Task f);

private:
    bool isFull() const;
    void runInThread();
    Task take();

    mutable MutexLock mutex_;
    Condition notEmpty_ GUARDED_BY(mutex_);
    Condition notFull_ GUARDED_BY(mutex_);
    std::string name_;
    Task threadInitCallback_;
    std::vector<std::unique_ptr<Thread>> threads_;
    std::deque<Task> queue_ GUARDED_BY(mutex_);
    size_t maxQueueSize_;
    bool running_;
};
