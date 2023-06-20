#pragma once 

#include "noncopyable.h"
#include "InetAddress.h"
#include "AbstractConnection.h"
#include "Helper.h"

class UdpConnection /*noncopyable*/ : public AbstractConnection
{
public:
    UdpConnection(EventLoop* loop,const std::string& name,int sockfd,const InetAddress& peerAddr);
    ~UdpConnection();

    void send(const std::string& buf);

private:
    void sendInLoop(const char *data, size_t len, int fd);
    void handleRead(Timestamp receiveTime);
    void handleWrite();

    // const InetAddress peerAddr_;
    
};
