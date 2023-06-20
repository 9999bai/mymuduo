#pragma once 

#include <functional>
#include <memory>

#include "noncopyable.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "Helper.h"
#include "Socket.h"
#include "Channel.h"


class AbstractConnection : noncopyable, public std::enable_shared_from_this<AbstractConnection>
{
public:
    AbstractConnection(EventLoop* loop,const std::string name,int sockfd, const InetAddress& peer);
    AbstractConnection(EventLoop* loop, const std::string& name,int sockfd);
    ~AbstractConnection();

    virtual void send(const std::string &buf);

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    
    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

    // const InetAddress localAddr() const { return localAddr_; }
    const InetAddress peerAddr() const { return peerAddr_; }

    void connectEstablished(); // 连接建立
    void connectDestroyed(); // 连接销毁
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) 
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

protected:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void setState(StateE s) { state_ = s; }
    
    void sendInLoop(const char *data, size_t len);

    virtual void handleRead(Timestamp receiveTime);
    virtual void handleWrite();
    void handleClose();
    void handleError();

    void shutdownInLoop();

    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_;       //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;// 消息发送完成以后的回调
    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;

    size_t highWaterMark_; // 高水位警戒线

    // const InetAddress localAddr_;
    const InetAddress peerAddr_;

    EventLoop* loop_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const std::string name_;
    StateE state_;

    std::mutex mutex_;
    Buffer inputBuffer_;    // 接收数据缓冲区
    Buffer outputBuffer_;   // 发生数据缓冲区
};