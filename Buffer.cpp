#include "Buffer.h"
#include <sys/uio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Logger.h"

ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536] = {0};
    struct iovec vec[2];
    
    const size_t writeable = writableBytes();// buffer_剩余可写空间
    
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writeable;
    
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    
    const int iovcnt = (writeable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    // ssize_t n = ::read(fd, extrabuf, sizeof(extrabuf));
    // LOG_INFO("readFd writableBytes = %d, recv size = %d\n", (int)writeable, (int)n);

    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if(n <= writeable) //buff_缓冲区足够存储读出来的数据
    {
        writerIndex_ += n;
    }
    else // buff_里面装满了，往extrabuf里面写入数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n-writeable); // 往extrabuf里面写 n-writeable 长度数据
    }
    return n;
}

ssize_t Buffer::readFd_udp(int fd, struct sockaddr *src_addr, socklen_t *addrlen, int* saveErrno)
{
    char extrabuf[65536] = {0};
    const size_t writeable = writableBytes();// buffer_剩余可写空间
    const ssize_t n = ::recvfrom(fd, begin() + writerIndex_, writeable+65536, 0, src_addr, addrlen);

    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if(n <= writeable) //buff_缓冲区足够存储读出来的数据
    {
        writerIndex_ += n;
    }
    else // buff_里面装满了，往extrabuf里面写入数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n-writeable); // 往extrabuf里面写 n-writeable 长度数据
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}

ssize_t Buffer::writeFd_udp(int fd, struct sockaddr *src_addr, socklen_t addrlen, int* saveErrno)
{
    // struct sockaddr_in send_addr;
    // send_addr.sin_family = AF_INET;
    // send_addr.sin_port = htons(8080);
    // send_addr.sin_addr.s_addr = inet_addr("192.168.2.102");
    // int addr_length = sizeof(send_addr);
    // if( sendto(sock_fd, buff, cs_pkg.GetCachedSize(), 0, (struct sockaddr *)&send_addr, sizeof(send_addr)) < 0 )
    ssize_t n = ::sendto(fd, peek(), readableBytes(), 0, src_addr, addrlen);

    // char buff[20] = "123456789";
    // size_t bufflen = sizeof buff;

    // ssize_t n = ::sendto(fd, buff, bufflen, 0, (struct sockaddr *)&send_addr, sizeof(send_addr));
    if(n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}