#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Helper.h"

#include <functional>
#include <atomic>
#include <memory>

class Channel;
class EventLoop;


class SerialConnector : noncopyable, public std::enable_shared_from_this<SerialConnector>
{
public:
    using NewConnectionCallback = std::function<void(int)>;

    SerialConnector(EventLoop* loop, const mymuduo::struct_serial& config);
    ~SerialConnector();

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }
    
    void start();
    void restart();
    void stop();

    const std::string& serialName() const { return serialConfig_.serialName; }

private:
    enum States { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30*1000; //30s
    static const int kInitRetryDelayMs = 500;   //0.5s

    void setState(States s) { state_ = s; }
    void startInLoop();
    void stopInLoop();

    bool setSerialPort(int sockfd);
    void open();
    void opening(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();

    EventLoop* loop_;
    // const std::string serialName_;
    const mymuduo::struct_serial& serialConfig_;
    std::atomic_bool connect_;
    std::atomic<States> state_;
    std::unique_ptr<Channel> channel_;
    int retryDelayMs_;
    NewConnectionCallback newConnectionCallback_;
};

