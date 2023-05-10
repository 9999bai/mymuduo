#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool resuport);
    ~Acceptor();
    
    void setNewConnectionCallback(const NewConnectionCallback& cb) { NewConnectionCallback_ = cb; }
    bool listenning() const { return listenning_; }
    void listen();
    
private:
    void handleRead();
    
    EventLoop* loop_; // Acceptor 用的就是用户定义的那个baseloop，也就是mainloop 
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback NewConnectionCallback_;
    bool listenning_;
};