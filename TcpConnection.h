#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"
#include "Helper.h"

#include <memory>
#include <string>
#include <atomic>
#include <mutex>

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer ==> Acceptor ==> 有一个新用户连接，通过accept函数拿到confd
 * ==>打包 Tcpconnection 设置相应的回调到Channel上，Channel注册到poller上
 * poller监测到事件，就会调用channel的回调操作
*/


class TcpConnection : noncopyable , public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                 const std::string name,
                 int sockfd,
                 const InetAddress& localAddr,
                 const InetAddress& peerAddr);

    TcpConnection(EventLoop* loop,
                 const std::string name,
                 int sockfd,
                 const InetAddress& peerAddr);

    TcpConnection(EventLoop* loop,
                 const std::string name,
                 int sockfd);
    
    ~TcpConnection();
    
    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddr() const { return localAddr_; }
    const InetAddress& peerAddr() const { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }
    bool disconnected() const { return state_ == kDisconnected; }

    void send(const std::string &buf);
    void send_udp(const std::string& buf);
    void shutdown(); // 关闭连接

    // void getMsg(std::shared_ptr<Buffer>& buff);//外部通过定时器获取inputBuffer_数据

    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) 
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }
    
    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();
    
private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    
    void setState(StateE s) { state_ = s; }
    
    void handleRead(Timestamp receiveTime);
    void handleRead_udp(Timestamp receiveTime);
    void handleWrite();
    void handleWrite_udp();
    void handleClose();
    void handleError();
    
    void sendInLoop(const char* data, size_t len);
    void sendInLoop_udp(const char* data, size_t len, int fd);
    void shutdownInLoop();
    
    EventLoop* loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;
    
    // 这里和Acceptor类似  Acceptor ==> mainLoop    TcpConnection ==> subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    
    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    
    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_;       //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;// 消息发送完成以后的回调
    CloseCallback closeCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    size_t highWaterMark_; // 高水位警戒线

    std::mutex mutex_;
    Buffer inputBuffer_;    // 接收数据缓冲区
    Buffer outputBuffer_;   // 发生数据缓冲区
    
};

