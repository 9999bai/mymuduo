#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include "Callbacks.h"
#include "TimerId.h"
#include "Logger.h"

#include <functional>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>

class TimerId;
class Poller;
class Channel;
class TimerQueue;

//时间循环类，主要包含两大模块  channel  poller（epoll）
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;
    
    EventLoop();
    ~EventLoop();
    
    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();
    
    Timestamp pollReturnTime() { return pollReturnTime_; }
    
    //  在当前loop中执行cb
    void runInLoop(Functor cb);
    //  把cb放入到队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    TimerId runAt(Timestamp time, TimerCallback cb);
    TimerId runAfter(double delay, TimerCallback cb);
    TimerId runEvery(double interval, TimerCallback cb);
    void cancel(TimerId timerId);

    // 用来唤醒loop所在线程
    void wakeup();
    
    // EventLoop的方法--> Poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);
    
    void assertInLoopThread()
    {
        if(!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }
    // 判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
    
private:
    void abortNotInLoopThread();
    void handleRead();  //   wake up
    void doPendingFunctors();  //  执行回调

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_;// 原子操作 通过CAS实现
    std::atomic_bool quit_;// 标识推出loop循环
    
    const pid_t threadId_;   //  记录当前loop所在线程的id
    
    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;

    int wakeupFd_; // 当mainloop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理
    std::unique_ptr<Channel> wakeupChannel_;
    
    ChannelList activeChannels_;

    // std::atomic_bool eventHandling_;
    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有回调操作
    std::mutex mutex_; //   互斥锁，用来保护上面vector容器的线程安全操作    
};