#include "UdpClient.h"

namespace detail_udp
{
    void removeConnection(EventLoop* loop, const ConnectionPtr& conn)
    {
        loop->queueInLoop(std::bind(&AbstractConnection::connectDestroyed, conn));
    }
}

UdpClient::UdpClient(EventLoop* loop, const std::string& name, const InetAddress& serveraddr, const std::string& localIp)
            : loop_(loop)
            , UDPconnector_(new UdpConnector(loop, serveraddr, localIp))
            , name_(name)
            // , serveraddr_(serveraddr)
            , retry_(false)
            , connect_(true)
            // , nextConnId_(1)
{
    UDPconnector_->setNewConnectionCallback(std::bind(&UdpClient::newConnection, this, std::placeholders::_1));
}

UdpClient::~UdpClient()
{
    ConnectionPtr conn;
    bool unique = false;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        unique = UDPconnection_.unique();
        conn = UDPconnection_;
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
        UDPconnector_->stop();
        // loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
    }
}

void UdpClient::connect()
{
    LOG_INFO("UdpClient::connect %s -- connecting to %s\n", name_.c_str(), UDPconnector_->serverAddress().toIpPort().c_str());
    connect_ = true;
    UDPconnector_->start();
}

void UdpClient::disconnect()
{
    connect_ = false;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if(UDPconnection_)
        {
            UDPconnection_->shutdown();
        }
    }
}

void UdpClient::stop()
{
    connect_ = false;
    UDPconnector_->stop();
}

ConnectionPtr UdpClient::connection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return UDPconnection_;
}

void UdpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    // InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32] = {0};
    snprintf(buf, sizeof buf, ":%s", UDPconnector_->serverAddress().toIpPort().c_str());
    std::string connName = name_ + buf;
    
    // ConnectionPtr conn(new UdpConnection(loop_, connName, sockfd, serveraddr_));
    ConnectionPtr conn = std::make_shared<UdpConnection>(loop_, connName, sockfd, UDPconnector_->serverAddress()); //(UdpConnection(loop_, connName, sockfd, serveraddr_));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&UdpClient::removeConnection, this, std::placeholders::_1));
    {
        std::unique_lock<std::mutex> lock(mutex_);
        UDPconnection_ = conn;
    }
    conn->connectEstablished();
}

void UdpClient::removeConnection(const ConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        UDPconnection_.reset();
    }
    loop_->queueInLoop(std::bind(&AbstractConnection::connectDestroyed, conn));
    if(retry_ && connect_)
    {
        LOG_INFO("TcpClient::connect %s Reconnect to %s\n", name_.c_str(), UDPconnector_->serverAddress().toIpPort().c_str());
        UDPconnector_->restart();
    }
}

