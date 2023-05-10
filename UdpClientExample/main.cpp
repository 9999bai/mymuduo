#include "/usr/include/mymuduo/UdpClient.h"

class testUdpClient
{
public:
    testUdpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& localIp)
            : loop_(loop), udpClient_(loop, "udpClient", serverAddr, localIp)
    {
        udpClient_.setConnectionCallback(std::bind(&testUdpClient::onConnect, this, std::placeholders::_1));
        udpClient_.setMessageCallback(std::bind(&testUdpClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        // udpClient_.enableRetry();
    }
    ~testUdpClient() { }

    void start() { udpClient_.connect(); }

private:
    void onConnect(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("onConnect : new connection [%s] from %s\n", conn->name().c_str(), conn->peerAddr().toIpPort().c_str());
            conn->send_udp("hello");
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
    }

    EventLoop* loop_;
    UdpClient udpClient_;
};

int main()
{
    EventLoop loop;
    InetAddress serverAddr(8080, "192.168.2.102");
    testUdpClient Client(&loop, serverAddr,"192.168.2.103");

    InetAddress serverAddr1(8081, "192.168.2.102");
    testUdpClient Client1(&loop, serverAddr1,"192.168.2.103");

    Client.start();
    Client1.start();

    loop.loop();
}
