#pragma once

#include <vector>
#include <string>
#include <sys/socket.h>

/** +-------------------+-------------------+-------------------+
 *  | prependable bytes |   readable bytes  |  writeable bytes  |
 *  |                   |    ( CONTENT )    |                   |
 *  +-------------------+-------------------+-------------------+
 *  |                   |                   |                   |
 *  0      <=     readerIndex    <=    writeIndex     <=      size
*/

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;
    
    explicit Buffer(size_t initialSize = kInitialSize)
                : buffer_(kCheapPrepend + kInitialSize)
                , readerIndex_(kCheapPrepend)
                , writerIndex_(kCheapPrepend)
    {}
    
    void swap(Buffer& rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    size_t readableBytes() const
    {   return writerIndex_ - readerIndex_; }
    
    size_t writableBytes() const
    {   return buffer_.size() - writerIndex_;   }
    
    size_t prependableBytes() const
    {   return readerIndex_;    }
    
    // 返回缓存区中可读数据的起始地址
    const char* peek() const
    { return begin() + readerIndex_; }
    
    // 复位
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        { readerIndex_ += len; }// 应用只读取了可读缓冲区数据一部分(len)，还剩下readerIndex += len --> writerIndex_
        else
        { retrieveAll(); }
    }
    
    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    std::vector<char> retrieveAllasVector()
    {
        std::string tmp = retrieveAllAsString();
        std::vector<char> v_data(tmp.begin(), tmp.end());
        return v_data;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // 应用可读取数据的长度
    }
    
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);  // 上一句把缓冲区可读数据，读取出来，这里要对缓冲区复位
        return result;
    }
    
    // 往缓冲区写数据  buffer_.size - writerIndex_ = 可写缓冲区长度
    void ensureWriteableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            makeSpace(len); // 扩容函数
        }
    }
    
    // 把【data, data+len】内存上的数据添加到writeable缓冲区当中
    void append(const char* data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }
    
    char* beginWrite()
    {
        return begin() + writerIndex_; 
    }
    
    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }
    
    // 从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    ssize_t readFd_udp(int fd, struct sockaddr *src_addr, socklen_t *addrlen, int* saveErrno);

    ssize_t writeFd(int fd, int* saveErrno);
    ssize_t writeFd_udp(int fd, struct sockaddr *src_addr, socklen_t addrlen, int* saveErrno);
    
private:
    char* begin() { return &*buffer_.begin(); } // vector底层数组首元素的地址，也就是数组的起始地址
    const char* begin() const { return &*buffer_.begin(); }
    // 扩容
    void makeSpace(size_t len)
    {
        if(writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }
    
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
    
};