#include "Helper.h"
#include "Logger.h"

#include <unistd.h>
#include <strings.h>
#include "EventLoop.h"

// namespace sockets
// {
int sockets::createNonblocking(sa_family_t family)
{
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL ("sockets::createNonblockingOrDie");
    }
    return sockfd;
}

int sockets::createblocking_udp()
{
    int sockfd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
    if (sockfd < 0)
    {
        int saveErr = errno;
        LOG_FATAL ("sockets::createNonblocking_udp errno=%d", saveErr);
    }
    return sockfd;
}

void sockets::close(int sockfd)
{
    if(::close(sockfd) < 0)
    {
        LOG_ERROR("sockets::close error");
    }
}

int sockets::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

struct sockaddr_in sockets::getLocalAddr(int sockfd)
{
    struct sockaddr_in localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if(::getsockname(sockfd, (sockaddr*)&localaddr, &addrlen) < 0)
    {
        LOG_ERROR("getLocalAddr -- sockets::getsockname error");
    }
    return localaddr;
}

struct sockaddr_in sockets::getPeerAddr(int sockfd)
{
    struct sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if(::getpeername(sockfd,(sockaddr*)&peeraddr, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getPeerAddr -- sockets::getsockname error");
    }
    return peeraddr;
}

bool sockets::isSelfConnect(int sockfd)
{
    struct sockaddr_in localaddr = sockets::getLocalAddr(sockfd);
    struct sockaddr_in peeraddr = sockets::getPeerAddr(sockfd);
    if(localaddr.sin_family == AF_INET)
    {
        return localaddr.sin_port == peeraddr.sin_port
            && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
    }
    return false;
}
// }

EventLoop* mymuduo::CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d CheckLoopNotNull loop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

