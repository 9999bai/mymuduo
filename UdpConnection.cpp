#include "UdpConnection.h"
#include "Channel.h"


UdpConnection::UdpConnection(EventLoop* loop,const std::string& name,int sockfd,const InetAddress& peerAddr)
                : AbstractConnection(mymuduo::CheckLoopNotNull(loop), name, sockfd, peerAddr)
                // , peerAddr_(peerAddr)
{
    channel_->setReadCallback(std::bind(&UdpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallabck(std::bind(&UdpConnection::handleWrite, this));
    channel_->setErrorCallback(std::bind(&UdpConnection::handleError, this));
    channel_->setCloseCallabck(std::bind(&UdpConnection::handleClose, this));
    LOG_INFO("UdpConnection::udp --- ctor[%s] at fd = %d \n", name.c_str(), sockfd);
    // LOG_INFO("UdpConnection::udp --- ctor %s \n", peerAddr_.toIpPort().c_str());
}

UdpConnection::~UdpConnection()
{

}

void UdpConnection::send(const std::string &buf)
{
    if(state_ == kConnected)
    {
        if(loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size(), socket_->fd());
        }
        else
        {
            loop_->runInLoop(std::bind(&UdpConnection::sendInLoop, this, buf.c_str(), buf.size(), socket_->fd()));
        }
    }
}

void UdpConnection::sendInLoop(const char *data, size_t len, int fd)
{
    ssize_t nwrote = 0;
    size_t remaing = len;
    bool faultError = false;
    
    if(state_ == kDisconnected)
    {
        LOG_INFO("UdpConnection::sendInLoop disConnected, give up writing");
        return;
    }
    // 表示channel第一次开始写数据， 而且缓冲区没有待发送数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = sendto(fd, data, len, 0, (sockaddr*)peerAddr_.getSockAddr(), sizeof(sockaddr_in));
        int saveErr = errno;
        LOG_INFO("UdpConnection sendto fd=%d,dataLen=%d,nwrote=%d, errno=%d", fd, len, nwrote, saveErr);

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
                LOG_ERROR("UdpConnection::sendInLoop_udp error=%d", saveErr);
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

void UdpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    InetAddress addr(peerAddr_.toPort());
    socklen_t len = sizeof *(addr.getSockAddr());
    ssize_t n = inputBuffer_.readFd_udp(channel_->fd(), (sockaddr*)addr.getSockAddr(), &len, &saveErrno);
    if(n > 0 && messageCallback_)
    {
        LOG_INFO("UdpConnection::handleRead...");
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
        LOG_ERROR("UdpConnection::handleRead errno=%d",saveErrno);
        handleError();
    }
}

void UdpConnection::handleWrite()
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
            LOG_ERROR("UdpConnection::handleWrite fd = %d,n=%d,error = %d", channel_->fd(),n , saveErrno);
        }
    }
    else
    {
        LOG_ERROR("UdpConnection fd=%d is down, no more writing\n", channel_->fd());
    }
}
