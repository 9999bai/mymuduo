#include "SerialPortConnection.h"
#include "EventLoop.h"
#include "Channel.h"


SerialPortConnection::SerialPortConnection(EventLoop* loop, const std::string& name,int sockfd)
                    : AbstractConnection(loop, name, sockfd)
{
    channel_->setReadCallback(std::bind(&SerialPortConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallabck(std::bind(&SerialPortConnection::handleWrite, this));
    channel_->setErrorCallback(std::bind(&SerialPortConnection::handleError, this));
    channel_->setCloseCallabck(std::bind(&SerialPortConnection::handleClose, this));
    LOG_INFO("SerialPort::ctor[%s] at fd = %d \n", name.c_str(), sockfd);
}

SerialPortConnection::~SerialPortConnection()
{

}

// void SerialPortConnection::handleClose()
// {
//     loop_->assertInLoopThread();
//     LOG_INFO("SerialPortConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
//     setState(kDisconnected);
//     channel_->disableAll();
    
//     TcpConnectionPtr connPtr(shared_from_this());
//     if(connectionCallback_)
//     {
//         connectionCallback_(connPtr); // 连接关闭的回调
//     }
//     if(closeCallback_)
//     {
//         closeCallback_(connPtr);    // 关闭连接的回调 执行的是TcpServer::removeConnection回调方法
//     }
// }

// void SerialPortConnection::send(const std::string &buf)
// {

// }

// void SerialPortConnection::sendInLoop(const char *data, size_t len)
// {

// }

// void SerialPortConnection::handleRead(Timestamp receiveTime)
// {
//     loop_->assertInLoopThread();
//     int saveErrno = 0;
//     ssize_t n;
//     {
//         std::unique_lock<std::mutex> lock(mutex_);
//         n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
//     }
//     if(n > 0)
//     {
//         // LOG_INFO("TcpConnection::handleRead...n=%d\n", n);
//         // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
//         if(messageCallback_)
//         {
//             messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
//         }
//     }
//     else if(n == 0)
//     {
//         handleClose();
//     }
//     else
//     {
//         errno = saveErrno;
//         handleError();
//     }
// }

// void SerialPortConnection::handleWrite()
// {

// }
