#pragma once

#include "noncopyable.h"
#include "TcpConnection.h"
#include "Connector.h"
#include "Buffer.h"
#include "Logger.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Helper.h"
#include "Callbacks.h"

#include <mutex>

// class Connector;

class TcpClient : noncopyable
{
public:
    using ConnectorPtr = std::shared_ptr<Connector>;

    TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection();

    EventLoop* getLoop() const { return loop_; }
    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }
    const std::string& name() const { return name_; }

    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = cb; }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = cb; }
    void setWriteComplateCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = cb; }

private:
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr& conn);

    EventLoop* loop_;
    ConnectorPtr connector_;
    const std::string name_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    
    std::atomic_bool retry_;
    std::atomic_bool connect_;

    int nextConnId_;
    std::mutex mutex_;
    TcpConnectionPtr connection_;
};