#pragma once

#include "noncopyable.h"
#include "TcpConnection.h"
#include "Buffer.h"
#include "Logger.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Helper.h"
#include "Callbacks.h"

#include <mutex>


class SerialPort
{
public:
    SerialPort(EventLoop* loop, const mymuduo::struct_serial& serialConfig);
    ~SerialPort();

    void connect();
    void disconnect();
    void stop();

    ConnectionPtr connection();

    EventLoop* getLoop() const { return loop_; }
    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }
    const std::string& name() const { return serialName_; }
    
    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = cb; }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = cb; }
    void setCloseCallback(CloseCallback cb) { closeCallback_ = cb; }
    void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback_(HighWaterMarkCallback cb) { highWaterMarkCallback_ = cb; }
    
private:
    void newConnection(int sockfd);
    void removeConnection(const ConnectionPtr& conn);

    EventLoop* loop_;
    ConnectorPtr connector_;
    const std::string serialName_;

    ConnectionCallback connectionCallback_;
    CloseCallback closeCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;

    std::atomic_bool retry_;
    std::atomic_bool connect_;

    int nextConnectId_;
    std::mutex mutex_;
    
    ConnectionPtr connection_; 
};

