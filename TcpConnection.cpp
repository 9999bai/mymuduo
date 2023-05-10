#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string>
#include <strings.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection loop is null\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}
//tcp
TcpConnection::TcpConnection(EventLoop* loop,
                const std::string name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr)
                : loop_(CheckLoopNotNull(loop))
                , name_(name)
                , state_(kConnecting)
                , reading_(true)
                , socket_(new Socket(sockfd))
                , channel_(new Channel(loop, sockfd))
                , localAddr_(localAddr)
                , peerAddr_(peerAddr)
                , highWaterMark_(HIGHWATER)
{
    // 给channel设置相应的回调函数， poller给channel通知感兴趣的事件发生了，channel会回调相应的操作函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallabck(std::bind(&TcpConnection::handleWrite, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    channel_->setCloseCallabck(std::bind(&TcpConnection::handleClose, this));
    LOG_INFO("TcpConnection::tcp  ctor[%s] at fd = %d \n", name.c_str(), sockfd);
    // socket_->setKeepAlive(true); // 保活机制
    // outputBuffer_.append("hello1111111", 12);
}

//udp
TcpConnection::TcpConnection(EventLoop* loop,
                const std::string name,
                int sockfd,
                const InetAddress& peerAddr)
                : loop_(CheckLoopNotNull(loop))
                , name_(name)
                , state_(kConnecting)
                , reading_(true)
                , socket_(new Socket(sockfd))
                , channel_(new Channel(loop, sockfd))
                , peerAddr_(peerAddr)
                , highWaterMark_(HIGHWATER)
{
    // 给channel设置相应的回调函数， poller给channel通知感兴趣的事件发生了，channel会回调相应的操作函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead_udp, this, std::placeholders::_1));
    channel_->setWriteCallabck(std::bind(&TcpConnection::handleWrite_udp, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    channel_->setCloseCallabck(std::bind(&TcpConnection::handleClose, this));
    LOG_INFO("TcpConnection::udp --- ctor[%s] at fd = %d \n", name.c_str(), sockfd);
    // socket_->setKeepAlive(true); // 保活机制
}

//serial
TcpConnection::TcpConnection(EventLoop* loop,
                const std::string name,
                int sockfd)
                : loop_(CheckLoopNotNull(loop))
                , name_(name)
                , state_(kConnecting)
                , reading_(true)
                , socket_(new Socket(sockfd))
                , channel_(new Channel(loop, sockfd))
                , highWaterMark_(HIGHWATER)
{
    // 给channel设置相应的回调函数， poller给channel通知感兴趣的事件发生了，channel会回调相应的操作函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallabck(std::bind(&TcpConnection::handleWrite, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    channel_->setCloseCallabck(std::bind(&TcpConnection::handleClose, this));
    LOG_INFO("SerialPort::ctor[%s] at fd = %d \n", name.c_str(), sockfd);
    // socket_->setKeepAlive(true); // 保活机制
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd = %d state = %d \n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::send(const std::string& buf)
{
    if (state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::send_udp(const std::string& buf)
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop_udp(buf.c_str(), buf.size(), socket_->fd());
        }
        else
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop_udp, this, buf.c_str(), buf.size(), socket_->fd()));
        }
    }
}

void TcpConnection::sendInLoop(const char* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaing = len;
    bool faultError = false;
    
    if(state_ == kDisconnected)
    {
        LOG_INFO("TcpConnection::sendInLoop  disConnected, give up writing");
        return;
    }
    // 表示channel第一次开始写数据， 而且缓冲区没有待发送数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = write(channel_->fd(), data, len);
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
        outputBuffer_.append(data + nwrote, remaing);
        if(!channel_->isWriting())
        {
            channel_->enableWriting(); // 这里注册channnel的写事件，否则poller不会给channel通知epollout
        }
    }
}

void TcpConnection::sendInLoop_udp(const char* data, size_t len, int fd)
{
    ssize_t nwrote = 0;
    size_t remaing = len;
    bool faultError = false;
    
    if(state_ == kDisconnected)
    {
        LOG_INFO("TcpConnection::sendInLoop disConnected, give up writing");
        return;
    }
    // 表示channel第一次开始写数据， 而且缓冲区没有待发送数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = sendto(fd, data, len, 0, (sockaddr*)peerAddr_.getSockAddr(), sizeof(sockaddr_in));
        int saveErr = errno;
        LOG_INFO("TcpConnection sendto fd=%d,dataLen=%d,nwrote=%d, errno=%d", fd, len, nwrote, saveErr);

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
                LOG_ERROR("TcpConnection::sendInLoop_udp error=%d", saveErr);
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
        outputBuffer_.append(data + nwrote, remaing);
        if(!channel_->isWriting())
        {
            channel_->enableWriting(); // 这里注册channnel的写事件，否则poller不会给channel通知epollout
        }
    }
}

 // 关闭连接
void TcpConnection::shutdown()
{
    if(state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting()) // 说明当前outputBuffer中的数据已经全部发送完成
    {
        socket_->shutdownWrite(); // 关闭写端
    }
}

// 连接建立
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();
    
    if(connectionCallback_)
    {
        connectionCallback_(shared_from_this());
    }
}

// 连接销毁
void TcpConnection::connectDestroyed()
{
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

// void TcpConnection::getMsg(std::shared_ptr<Buffer>& buff)
// {
//     {
//         std::unique_lock<std::mutex> lock(mutex_);
//         buff->append(inputBuffer_.retrieveAllAsString().c_str(), inputBuffer_.readableBytes());
//     }
// }

void TcpConnection::handleRead(Timestamp receiveTime)
{
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

void TcpConnection::handleWrite()
{
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
            LOG_ERROR("TcpConnection::handleWrite error=%d", saveErrno);
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing\n", channel_->fd());
    }
}

void TcpConnection::handleRead_udp(Timestamp receiveTime)
{
    int saveErrno = 0;
    InetAddress addr(peerAddr_.toPort());
    socklen_t len = sizeof *(addr.getSockAddr());
    ssize_t n = inputBuffer_.readFd_udp(channel_->fd(), (sockaddr*)addr.getSockAddr(), &len, &saveErrno);
    if(n > 0 && messageCallback_)
    {
        LOG_INFO("TcpConnection::handleRead_udp...");
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead_udp errno=%d",saveErrno);
        handleError();
    }
}

void TcpConnection::handleWrite_udp()
{
    if(channel_->isWriting())
    {
        int saveErrno = 0;
        socklen_t len = sizeof(*(peerAddr_.getSockAddr()));
        ssize_t n = outputBuffer_.writeFd_udp(channel_->fd(), (sockaddr*)(peerAddr_.getSockAddr()), len, &saveErrno);
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
            LOG_ERROR("TcpConnection::handleWrite_udp fd = %d,n=%d,error = %d", channel_->fd(),n , saveErrno);
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing\n", channel_->fd());
    }
}

// poller:EPOLLHUP == > channel::closeCallback  ==> TcpConnection::handleClose
void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();
    
    TcpConnectionPtr connPtr(shared_from_this());
    if(connectionCallback_)
    {
        connectionCallback_(connPtr); // 连接关闭的回调
    }
    if(closeCallback_)
    {
        closeCallback_(connPtr);    // 关闭连接的回调 执行的是TcpServer::removeConnection回调方法
    }
}

void TcpConnection::handleError()
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
    LOG_ERROR("TcpConnection::handleError name:%s  SO_ERROR:%d \n", name_.c_str(), err);
}

