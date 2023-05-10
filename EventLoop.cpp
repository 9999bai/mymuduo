#include "EventLoop.h"
#include "Poller.h"
#include "Channel.h"
#include "DefaultPoller.h"
#include "TimerQueue.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>


// 防止一个线程创建多个EventLoop
__thread EventLoop* t_loopInThisThread = nullptr;

//定义默认的Poller IO复用接口的超时时间
const int kPollerTimeMs = 50000;

// 创建wakeupfd用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
            : looping_(false)
            , quit_(false)
            // , eventHandling_(false)
            , callingPendingFunctors_(false)
            , threadId_(CurrentThread::tid())
            , poller_(Poller::newDefaultPoller(this))
            , timerQueue_(new TimerQueue(this))
            , wakeupFd_(createEventfd())
            , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_INFO("EventLoop create %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }
    
    // 设置wakeupFd的事件类型 以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventloop都监听wakeupChannel的EPOLLIN的读事件了
    LOG_INFO("wakeupChannel enableReading....");
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping \n", this);
    
    while(!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollerTimeMs, &activeChannels_);
        // LOG_INFO("poll -- activeChannels_.size=%d", activeChannels_.size());

        for(Channel* channel : activeChannels_)
        {
            //Poller监听哪些channel发生事件了，然后上报EventLoop,通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping \n", this);
    looping_ = false;
}

// 退出事件循环
void EventLoop::quit()
{
    quit_ = true;
    if(!isInLoopThread())
    {
        wakeup();
    }
}

//  在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())// 在当前线程中执行cb
    {
        cb();
    }
    else// 在非当前loop线程中执行cb，就需要唤醒loop所在线程执行cb
    {
        // LOG_INFO("EventLoop  queueInLoop");
        queueInLoop(cb);
    }
}

//  把cb放入到队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // 唤醒相应的，需要执行上面回调操作的loop的线程了
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();//唤醒loop所在线程
    }
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

// 用来唤醒loop所在线程, 向wakeupfd 写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup write %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法--> Poller的方法
void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}

//  执行回调
void EventLoop::doPendingFunctors()  
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    
    for( const Functor &functor: functors )
    {
        functor();// 执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL("EventLoop::abortNotInLoopThread - EventLoop was created in threadId_ != current thread id ");
}