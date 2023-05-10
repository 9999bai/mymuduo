#include "UdpConnector.h"
#include "Logger.h"
#include "Socket.h"

UdpConnector::UdpConnector(EventLoop* loop, const InetAddress& serveraddr, const std::string& localIp)
            : loop_(loop)
            , serverAddr_(serveraddr)
            , localaddr_(InetAddress(serveraddr.toPort(),localIp))
            , connect_(false)
            , state_(kDisconnected)
            , socket_(new Socket(sockets::createblocking_udp()))
{

}

UdpConnector::~UdpConnector()
{

}

void UdpConnector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&UdpConnector::startInLoop, this));
}

void UdpConnector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    // retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void UdpConnector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&UdpConnector::stopInLoop, this));
}

void UdpConnector::startInLoop()
{
    loop_->assertInLoopThread();
    if(connect_)
    {
        connect();
    }
    else
    {
        LOG_INFO("UdpConnector do not connect");
    }
}


void UdpConnector::connect()
{
    socket_->bindAddress(localaddr_);
    setState(kConnecting);
    channel_.reset(new Channel(loop_, socket_->fd()));
    channel_->setWriteCallabck(std::bind(&UdpConnector::handleWrite, this));
    channel_->setErrorCallback(std::bind(&UdpConnector::handleError, this));
    LOG_INFO("UdpConnector::connect  channel_->enableWriting...");
    channel_->enableWriting();
}

void UdpConnector::stopInLoop()
{
    loop_->assertInLoopThread();
    if(state_ == kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void UdpConnector::handleWrite()
{
    if(state_ == kConnecting)
    {
        // channel_->disableWriting();
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if(err)
        {
            LOG_ERROR("UdpConnector::handleWrite errno=%d\n", err);
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if(connect_&& newConnectionCallback_)
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

void UdpConnector::handleError()
{
    LOG_ERROR("UdpConnector::handleError state=%d\n", (int)state_);
    if(state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_INFO("UdpConnector::handleError err=%d\n",err);
        retry(sockfd);
    }
}

void UdpConnector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(kDisconnected);
    if(connect_)
    {
        LOG_INFO("UdpConnector::retry Retry connecting to %s\n", serverAddr_.toIpPort().c_str());
        startInLoop();        
    }
    else
    {
        LOG_INFO("UdpConnector do not connect");
    }
}

int UdpConnector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    loop_->queueInLoop(std::bind(&UdpConnector::resetChannel, this));
    return sockfd;
}

void UdpConnector::resetChannel()
{
    channel_.reset();
}


