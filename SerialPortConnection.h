#pragma once
 
#include "AbstractConnection.h"

class SerialPortConnection : /*noncopyable ,*/ public AbstractConnection
{
public:
    SerialPortConnection(EventLoop* loop, const std::string& name, int sockfd);
    ~SerialPortConnection();

//     void send(const std::string &buf) override;

private:
    // void handleClose();
    //     void sendInLoop(const char *data, size_t len);
    // void handleRead(Timestamp receiveTime);
    //     void handleWrite();
};
