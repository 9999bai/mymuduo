#include "AbstractConnection.h"

AbstractConnection::AbstractConnection(EventLoop* loop,const std::string name,int sockfd, const InetAddress& peer)
                    : loop_(mymuduo::CheckLoopNotNull(loop))
                    , name_(name)
                    , state_(kConnecting)
                    , socket_(new Socket(sockfd))
                    , channel_(new Channel(loop, sockfd))
                    , highWaterMark_(HIGHWATER)
                    , peerAddr_(peer)
                    // , localAddr_(local)
{
    LOG_INFO("AbstractConnection ctor1 ... ");
}

AbstractConnection::AbstractConnection(EventLoop* loop, const std::string& name,int sockfd)
                : loop_(mymuduo::CheckLoopNotNull(loop))
                , name_(name)
                , state_(kConnecting)
                , socket_(new Socket(sockfd))
                , channel_(new Channel(loop, sockfd))
                , highWaterMark_(HIGHWATER)
{
    LOG_INFO("AbstractConnection ctor2 ... ");
}

AbstractConnection::~AbstractConnection()
{

}

void AbstractConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_INFO("SerialPortConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();
    
    ConnectionPtr connPtr(shared_from_this());
    if(connectionCallback_)
    {
        connectionCallback_(connPtr); // 连接关闭的回调
    }
    if(closeCallback_)
    {
        closeCallback_(connPtr);    // 关闭连接的回调 执行的是TcpServer::removeConnection回调方法
    }
}

void AbstractConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("%s::handleError SO_ERROR:%d \n", name_.c_str(), err);
}

void AbstractConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();
    
    if(connectionCallback_)
    {
        connectionCallback_(shared_from_this());
    }
}

void AbstractConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if(state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 把channel的所有感兴趣的事件，从poller中del掉
        if(connectionCallback_)
        {
            connectionCallback_(shared_from_this());
        }
    }
    channel_->remove(); // 把channel从poller中删除掉
}

void AbstractConnection::shutdown()
{
    if(state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&AbstractConnection::shutdownInLoop, shared_from_this()));
    }
}

void AbstractConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if(!channel_->isWriting()) // 说明当前outputBuffer中的数据已经全部发送完成
    {
        socket_->shutdownWrite(); // 关闭写端
    }
}

void AbstractConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&AbstractConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void AbstractConnection::sendInLoop(const char* data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaing = len;
    bool faultError = false;
    
    if(state_ == kDisconnected)
    {
        LOG_INFO("%s::sendInLoop  disConnected, give up writing", name_.c_str());
        return;
    }
    // 表示channel第一次开始写数据， 而且缓冲区没有待发送数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote >= 0)
        {
            remaing = len - nwrote;
            if(remaing == 0 && writeCompleteCallback_)
            {
                // 既然在这里数据全部发送完成，就不用再给channel设置epollout事件了
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK) // EWOULDBLOCK:非阻塞模式下由于没有数据，正常的返回
            {
                if(errno == EPIPE || errno == ECONNRESET) // SIGPIPE  RESET
                {
                    faultError = true;
                }
            }
        }
    }
    /** 当前这一次write,并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，
     * 然后给channel注册epollerout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock-channel
     * 调用writeCallback_回调方法,也就是TcpConnection::handleWrite方法，把发送缓冲区中的数据全部发送完成
    */
    if(!faultError && remaing > 0)
    {
        // 目前发送缓冲区待发送数据的长度
        size_t oldlen = outputBuffer_.readableBytes();
        if(oldlen + remaing >= highWaterMark_
            && oldlen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldlen + remaing));
        }
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaing);
        if(!channel_->isWriting())
        {
            channel_->enableWriting(); // 这里注册channnel的写事件，否则poller不会给channel通知epollout
        }
    }
}

void AbstractConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int saveErrno = 0;
    ssize_t n;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    }
    if(n > 0)
    {
        // LOG_INFO("TcpConnection::handleRead...n=%d\n", n);
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        if(messageCallback_)
        {
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        }
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        handleError();
    }
}

void AbstractConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if(channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if(n > 0)
        {
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if(writeCompleteCallback_)
                {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if(state_ == kDisconnecting)
                {
                    shutdownInLoop(); //在当前所属的loop中删除tcpconnection
                }
            }
        }
        else
        {
            LOG_ERROR("%s::handleWrite error=%d", name_.c_str(), saveErrno);
        }
    }
    else
    {
        LOG_ERROR("%s fd=%d is down, no more writing\n", name_.c_str(), channel_->fd());
    }
}

