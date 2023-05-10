#include "Connector.h"

#include "Logger.h"
#include "Channel.h"
#include "EventLoop.h"

#include <errno.h>

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
            : loop_(loop)
            , serverAddr_(serverAddr)
            , connect_(false)
            , state_(kDisconnected)
            , retryDelayMs_(kInitRetryDelayMs)
{

}

Connector::~Connector()
{

}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    if(connect_)
    {
        connect();
    }
    else
    {
        LOG_INFO("do not connect");
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if(state_ == kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect()
{
    int sockfd = sockets::createNonblocking(serverAddr_.getSockAddr()->sin_family);
    int ret = ::connect(sockfd, (const struct sockaddr*)serverAddr_.getSockAddr(), sizeof serverAddr_);
    int saveErrno = (ret == 0) ? 0 : errno;
    switch (saveErrno)
    {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
      break;
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            LOG_INFO("retry...");
            LOG_INFO("Connector::connect() saveErrno = %d\n", saveErrno);
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_ERROR("connect error in Connector::startInLoop sockfd = %d, errno = %d\n", sockfd, saveErrno);
            sockets::close(sockfd);
      break;

        default:
            LOG_ERROR("Unexpected error in Connector::startInLoop sockfd = %d, errno = %d\n", sockfd, saveErrno);
            sockets::close(sockfd);
            // connectErrorCallback_();
        break;
    }
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallabck(std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallback(std::bind(&Connector::handleError, this));
    channel_->enableWriting();
}

void Connector::handleWrite()
{
    if(state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if(err)
        {
            LOG_ERROR("Connector::getSocketError errno=%d\n", err);
            retry(sockfd);
        }
        // else if(sockets::isSelfConnect(sockfd))
        // {
        //     LOG_INFO("Connector::handleWrite - Self connect");
        //     retry(sockfd);
        // }
        else
        {
            setState(kConnected);
            if(connect_ && newConnectionCallback_)
            {
                newConnectionCallback_(sockfd);
            }
            else
            {
                sockets::close(sockfd);
            }
        }
    }
}

void Connector::handleError()
{
    LOG_ERROR("Connector::handleError state=%d\n", (int)state_);
    if(state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_INFO("Connector::handleError err=%d\n",err);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(kDisconnected);
    if(connect_)
    {
        LOG_INFO("Connector::retry Retry connecting to %s\n", serverAddr_.toIpPort().c_str());
        loop_->runAfter(retryDelayMs_/1000.0, std::bind(&Connector::startInLoop, shared_from_this()));
        // startInLoop();
        retryDelayMs_ = std::min(retryDelayMs_ *2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_INFO("do not connect");
    }
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}
