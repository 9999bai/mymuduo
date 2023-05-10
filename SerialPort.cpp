#include "SerialPort.h"


namespace detail
{
    void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
    {
        loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

SerialPort::SerialPort(EventLoop* loop, const mymuduo::struct_serial& serialConfig)
            : loop_(loop)
            , connector_(std::make_shared<SerialConnector>(loop,serialConfig))
            , serialName_(serialConfig.serialName)
            , retry_(false)
            , connect_(true)
            , nextConnectId_(1)
{
    LOG_INFO("SerialPort  ctor ...");
    connector_->setNewConnectionCallback(std::bind(&SerialPort::newConnection, this, std::placeholders::_1));
}

SerialPort::~SerialPort()
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
        CloseCallback cb = std::bind(&detail::removeConnection, loop_, std::placeholders::_1);
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

void SerialPort::connect()
{
    LOG_INFO("SerialPort::connecting to [%s]....\n", serialName_.c_str());
    connect_ = true;
    connector_->start();
}

void SerialPort::disconnect()
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

void SerialPort::stop()
{
    connect_ = false;
    connector_->stop();
}

TcpConnectionPtr SerialPort::connection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return connection_;
}

void SerialPort::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    // InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32] = {0};
    snprintf(buf, sizeof buf, ":%s#%d", serialName_.c_str(), nextConnectId_);
    ++nextConnectId_;
    std::string connName = buf;

    // TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd));
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, connName, sockfd);
    // new TcpConnection(loop_, connName, sockfd);
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&SerialPort::removeConnection, this, std::placeholders::_1));
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void SerialPort::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connection_.reset();
    }
    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if(retry_ && connect_)
    {
        LOG_INFO("SerialPort:: %s Reconnect----\n", serialName_.c_str());
        connector_->restart();
    }
}
