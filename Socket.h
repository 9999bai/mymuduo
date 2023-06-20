#pragma once

#include "noncopyable.h"
// #include "InetAddress.h"
class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {}
    ~Socket();
    
    int fd() { return sockfd_; }
    bool bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress *peeraddr);
    
    void shutdownWrite();
    
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
    
private:
    int sockfd_;
};