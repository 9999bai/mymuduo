#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Helper.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"

#include <functional>
#include <atomic>
#include <memory>


class UdpConnector : noncopyable, public std::enable_shared_from_this<UdpConnector>
{
public:
    using NewConnectionCallback = std::function<void(int)>;
    
    UdpConnector(EventLoop* loop, const InetAddress& serveraddr, const std::string& localIp);
    ~UdpConnector();

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }
    
    void start();
    void restart();
    void stop();

    const InetAddress& serverAddress() const { return serverAddr_; }

private:
    enum States { kDisconnected, kConnecting, kConnected };
    void setState(States s) { state_ = s; }

    void startInLoop();
    void stopInLoop();
    void connect();
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();

    EventLoop* loop_;
    const InetAddress serverAddr_;
    InetAddress localaddr_;
    std::atomic_bool connect_;
    std::atomic<States> state_;
    std::unique_ptr<Channel> channel_;
    std::unique_ptr<Socket> socket_;
    // int retryDelayMs_;
    NewConnectionCallback newConnectionCallback_;
};
