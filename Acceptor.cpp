#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s create listen sockfd err:%d\n", __FILE__, __FUNCTION__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool resuport)
        : loop_(loop)
        , acceptSocket_(createNonblocking())
        , acceptChannel_(loop, acceptSocket_.fd())
        , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(resuport);
    acceptSocket_.bindAddress(listenAddr);
    // 有新用户连接，执行回调
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading(); // acceptChannel==>epoll
}

//listenfd有事件发生了， 就是有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0)
    {
        if(NewConnectionCallback_)
        {
            NewConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            LOG_INFO("Acceptor::handleRead no NewConnectionCallback_ -- close");
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s accept err:%d\n", __FILE__, __FUNCTION__, errno);
        if(errno == EMFILE)
        {
            LOG_ERROR("%s:%s sockfd reached limit\n", __FILE__, __FUNCTION__);
        }
    }
}

