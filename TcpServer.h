#pragma once

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "Buffer.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPoll.h"
#include "TcpConnection.h"
#include "Callbacks.h"

enum Option
{
    kNoReusePort,
    kReusePort
};

class TcpServer : noncopyable
{
public:
    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string nameArg, Option option = kReusePort);
    ~TcpServer();
    
    //  设置底层subloop个数
    void setThreadNum(int numThreads);
    void setThreadInidCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback_(const HighWaterMarkCallback& cb) { highWaterMarkCallback_ = cb; }
    
    // 开启服务器监听
    void start();
    
private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);
    
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop* loop_;//baseLoop,用户定义的loop
    const std::string ipPort_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;    //运行在mainloop，监听新连接事件
    std::shared_ptr<EventLoopThreadPoll> threadPool_;
    
    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_;       //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;// 消息发送完成以后的回调
    HighWaterMarkCallback highWaterMarkCallback_; // 高水位警报回调
    
    ThreadInitCallback threadInitCallback_; // loop线程初始化的回调
    
    std::atomic_int started_;
    int nextConId_;
    ConnectionMap connections_; // 保存所有的连接
};