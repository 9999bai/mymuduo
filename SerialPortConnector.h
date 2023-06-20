#pragma once

#include "AbstractConnector.h"
#include "noncopyable.h"
// int BaudRateArr_[8] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200 };
// int SpeedArr_[8] = { 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200 };


class SerialPortConnector : public AbstractConnector
{
public:
    SerialPortConnector(EventLoop* loop, const mymuduo::struct_serial& config);
    ~SerialPortConnector();
    
private:
    bool setSerialPort(int sockfd);
    void handleWrite();
    void handleError();
    void connect();
    void connecting(int sockfd);

    mymuduo::struct_serial serialConfig_;

};



// class SerialConnector : noncopyable, public std::enable_shared_from_this<SerialConnector>
// {
// public:
//     using NewConnectionCallback = std::function<void(int)>;

//     SerialConnector(EventLoop* loop, const mymuduo::struct_serial& config);
//     ~SerialConnector();

//     void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }
    
//     void start();
//     void restart();
//     void stop();

//     const std::string& serialName() const { return serialConfig_.serialName; }

// private:
//     enum States { kDisconnected, kConnecting, kConnected };
//     static const int kMaxRetryDelayMs = 30*1000; //30s
//     static const int kInitRetryDelayMs = 500;   //0.5s

//     void setState(States s) { state_ = s; }
//     void startInLoop();
//     void stopInLoop();

//     bool setSerialPort(int sockfd);
//     void open();
//     void opening(int sockfd);
//     void handleWrite();
//     void handleError();
//     void retry(int sockfd);
//     int removeAndResetChannel();
//     void resetChannel();

//     EventLoop* loop_;
//     // const std::string serialName_;
//     const mymuduo::struct_serial& serialConfig_;
//     std::atomic_bool connect_;
//     std::atomic<States> state_;
//     std::unique_ptr<Channel> channel_;
//     int retryDelayMs_;
//     NewConnectionCallback newConnectionCallback_;
// };

