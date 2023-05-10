#include "UdpClient.h"

namespace detail_udp
{
    void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
    {
        loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

UdpClient::UdpClient(EventLoop* loop, const std::string& name, const InetAddress& serveraddr, const std::string& localIp)
            : loop_(loop)
            , connector_(new UdpConnector(loop, serveraddr, localIp))
            , name_(name)
            , serveraddr_(serveraddr)
            , retry_(false)
            , connect_(true)
            // , nextConnId_(1)
{
    connector_->setNewConnectionCallback(std::bind(&UdpClient::newConnection, this, std::placeholders::_1));
}

UdpClient::~UdpClient()
{
        TcpConnectionPtr conn;
    bool unique = false;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if(conn)
    {
        CloseCallback cb = std::bind(&detail_udp::removeConnection, loop_, std::placeholders::_1);
        if(unique)
        {
            // conn->forceClose();
        }
    }
    else
    {
        connector_->stop();
        // loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
    }
}

void UdpClient::connect()
{
    LOG_INFO("UdpClient::connect %s -- connecting to %s\n", name_.c_str(), connector_->serverAddress().toIpPort().c_str());
    connect_ = true;
    connector_->start();
}

void UdpClient::disconnect()
{
    connect_ = false;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if(connection_)
        {
            connection_->shutdown();
        }
    }
}

void UdpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

TcpConnectionPtr UdpClient::connection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return connection_;
}

void UdpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    // InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32] = {0};
    snprintf(buf, sizeof buf, ":%s", serveraddr_.toIpPort().c_str());
    std::string connName = name_ + buf;

    // sockaddr_in addr = sockets::getLocalAddr(sockfd);
    // InetAddress localAddr(addr);
    // InetAddress localAddr(sockets::getLocalAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, serveraddr_));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&UdpClient::removeConnection, this, std::placeholders::_1));
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void UdpClient::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connection_.reset();
    }
    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if(retry_ && connect_)
    {
        LOG_INFO("TcpClient::connect %s Reconnect to %s\n", name_.c_str(), connector_->serverAddress().toIpPort().c_str());
        connector_->restart();
    }
}

