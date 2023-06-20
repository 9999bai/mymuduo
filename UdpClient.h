#pragma once

#include "noncopyable.h"
#include "EventLoop.h"
#include "UdpConnector.h"
#include "UdpConnection.h"
#include "Logger.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Helper.h"

#include <mutex>
#include <atomic>
#include <functional>

class UdpClient : noncopyable
{
public:
    // using UdpConnectorPtr = std::shared_ptr<UdpConnector>;

    UdpClient(EventLoop* loop, const std::string& name, const InetAddress& serveraddr, const std::string& localIp);
    ~UdpClient();

    void connect();
    void disconnect();
    void stop();
    ConnectionPtr connection();

    EventLoop* getLoop() const { return loop_; }
    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }
    const std::string& name() const { return name_; }

    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = cb; }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = cb; }
    void setWriteComplateCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = cb; }

private:
    void newConnection(int sockfd);
    void removeConnection(const ConnectionPtr& conn);

    EventLoop* loop_;
    ConnectorPtr UDPconnector_;
    const std::string name_;
    // const InetAddress serveraddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    std::atomic_bool retry_;
    std::atomic_bool connect_;

    // int nextConnId_;
    std::mutex mutex_;
    ConnectionPtr UDPconnection_;
};
