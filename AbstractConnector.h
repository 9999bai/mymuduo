#pragma once 

#include <functional>
#include <atomic>
#include <memory>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Helper.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Logger.h"


class AbstractConnector : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int)>;

    AbstractConnector(EventLoop* loop, const std::string& name);
    AbstractConnector(EventLoop* loop, const InetAddress& serverAddr);
    virtual ~AbstractConnector();

    void setNewConnectionCallback(NewConnectionCallback cb) { newConnectionCallback_ = cb; }

    virtual void connect() = 0;

    const InetAddress serverAddress() const { return serverAddr_; }
    void start();
    void restart();
    void stop();

protected:
    enum States { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30*1000; //30s
    static const int kInitRetryDelayMs = 500;   //0.5s

    static int BaudRateArr_[8];
    static int SpeedArr_[8];


    void setState(States s) { state_ = s; }
    void startInLoop();
    void stopInLoop();

    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();

    const InetAddress serverAddr_;

    EventLoop* loop_;
    std::string name_;
    States state_;
    int retryDelayMs_;
    std::atomic_bool connect_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback newConnectionCallback_;
};

