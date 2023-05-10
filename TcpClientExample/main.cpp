#include "/usr/include/mymuduo/TcpClient.h"

class testTcpClient
{
public:
    testTcpClient(EventLoop* loop, const InetAddress& serverAddr)
        : loop_(loop), tcpclient_(loop, serverAddr, "test")
    {
        tcpclient_.setConnectionCallback(std::bind(&testTcpClient::onConnect, this, std::placeholders::_1));
        tcpclient_.setMessageCallback(std::bind(&testTcpClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        tcpclient_.enableRetry();
    }

    ~testTcpClient(){}
    void connect() { tcpclient_.connect(); }

private:

    void onConnect(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("onConnect : new connection [%s] from %s\n", conn->name().c_str(), conn->peerAddr().toIpPort().c_str());
            conn->send("hello !!!");
        }
        else
        {
            LOG_INFO("onConnect : [%s] connection [%s] is down\n", conn->peerAddr().toIpPort().c_str(), conn->name().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        LOG_INFO("onMessage connection[%s] RX : %s ",conn->peerAddr().toIpPort().c_str(), msg.c_str());
        conn->send(msg);
    }

    TcpClient tcpclient_;
    EventLoop* loop_;
};

int main()
{
    EventLoop loop;
    InetAddress serverAddr(8000, "192.168.2.102");
    testTcpClient client1(&loop, serverAddr);
    client1.connect();

    // InetAddress serverAddr(8000, "192.168.2.102");
    testTcpClient client2(&loop, serverAddr);
    client2.connect();

    // InetAddress serverAddr(8000, "192.168.2.102");
    // testTcpClient client3(&loop, serverAddr);
    // client3.connect();

    loop.loop();
}