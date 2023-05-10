// #include "/usr/include/mymuduo/TcpServer.h"
// #include "./../../../usr/include/mymuduo/TcpServer.h"
#include "/usr/include/mymuduo/TcpServer.h"
#include <string>

class EchServer
{
public:
    EchServer(EventLoop* loop,const InetAddress& addr,const std::string& name, bool iskReusePort = true)
              : server_(loop, addr, name, (iskReusePort==true)?kReusePort:kNoReusePort), loop_(loop)
    {
        // 注册回调
        server_.setConnectionCallback(std::bind(&EchServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        
        // 设置合适的线程数量
        server_.setThreadNum(3);
    }
    
    void start() { server_.start(); }
    
private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("onConnection - conn UP: %s", conn->peerAddr().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("onConnection - conn DOWN: %s", conn->peerAddr().toIpPort().c_str());
        }
    }
    
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        // conn->shutdown();
    }
    
    EventLoop* loop_;
    TcpServer server_;
};


int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    EchServer server(&loop, addr,"EchServer-01");// Accept non-blocking  listenfd create  bind
    server.start(); // 进入listen状态  loopthread  listenfd ==> acceptChannel ==> mainLoop
    loop.loop(); // 启动mainLoop的底层poller
    
    return 0;
}