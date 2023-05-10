#include "TcpClient.h"

namespace detail2
{
    void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
    {
        loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& name)
            : loop_(loop)
            , connector_(new Connector(loop, serverAddr))
            , name_(name)
            , retry_(false)
            , connect_(true)
            , nextConnId_(1)
{
    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
}

TcpClient::~TcpClient()
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
        CloseCallback cb = std::bind(&detail2::removeConnection, loop_, std::placeholders::_1);
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

void TcpClient::connect()
{
    LOG_INFO("TcpClient::connect %s -- connecting to %s\n", name_.c_str(), connector_->serverAddress().toIpPort().c_str());
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
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

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

TcpConnectionPtr TcpClient::connection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return connection_;
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32] = {0};
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    sockaddr_in addr = sockets::getLocalAddr(sockfd);
    InetAddress localAddr(addr);
    // InetAddress localAddr(sockets::getLocalAddr(sockfd));
    TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
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
