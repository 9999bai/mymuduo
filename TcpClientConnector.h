#pragma once 

#include "AbstractConnector.h"

class TcpClientConnector : public AbstractConnector
{
public:
    TcpClientConnector(EventLoop* loop, const InetAddress& serverAddr);
    ~TcpClientConnector();
    
private:
    void handleWrite();
    void handleError();
    void connect();
    void connecting(int sockfd);

    // const InetAddress serverAddr_;

};



// class Connector : noncopyable, public std::enable_shared_from_this<Connector>
// {
// public:
//     using NewConnectionCallback = std::function<void(int)>;

//     Connector(EventLoop* loop, const InetAddress& serverAddr);
//     ~Connector();

//     void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }
    
//     void start();
//     void restart();
//     void stop();

//     const InetAddress& serverAddress() const { return serverAddr_; }

// private:
//     enum States { kDisconnected, kConnecting, kConnected };
//     static const int kMaxRetryDelayMs = 30*1000; //30s
//     static const int kInitRetryDelayMs = 500;   //0.5s
    
//     void setState(States s) { state_ = s; }
//     void startInLoop();
//     void stopInLoop();
//     void connect();
//     void connecting(int sockfd);
//     void handleWrite();
//     void handleError();
//     void retry(int sockfd);
//     int removeAndResetChannel();
//     void resetChannel();

//     EventLoop* loop_;
//     const InetAddress serverAddr_;
//     std::atomic_bool connect_;
//     States state_;
//     std::unique_ptr<Channel> channel_;
//     int retryDelayMs_;
//     NewConnectionCallback newConnectionCallback_;
// };