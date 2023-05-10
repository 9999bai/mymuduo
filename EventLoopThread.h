#pragma once 

#include "noncopyable.h"
#include "Thread.h"
#include "Callbacks.h"

#include <functional>
#include <mutex>
#include <condition_variable>   //条件变量

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    // using ThreadInitCallback = std::function<void(EventLoop*)>;
    
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string& name = std::string());
    ~EventLoopThread();
    
    EventLoop* startLoop();
private:
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    
    ThreadInitCallback callback_;    
};