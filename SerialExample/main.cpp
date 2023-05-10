#include "/usr/include/mymuduo/SerialPort.h"
// #include "Helper.h"
#include <string>
#include <list>

class testSerialPort
{
public:
    testSerialPort(EventLoop* loop, const mymuduo::struct_serial& serialConfig)
            : serialport_(loop, serialConfig)
            , loop_(loop)
    {
        serialport_.setConnectionCallback(std::bind(&testSerialPort::onConnection, this, std::placeholders::_1));
        serialport_.setMessageCallback(std::bind(&testSerialPort::onMessage, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3));
        serialport_.enableRetry();
    }
    ~testSerialPort() { }

    void start() { serialport_.connect(); }

private:

    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("onConnection - conn UP: %s", conn->name().c_str());
            conn->send("hello !!!");
        }
        else
        {
            LOG_INFO("onConnection - conn DOWN: %s", conn->name().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        LOG_INFO("onMessage connection[%s] RX : %s ",conn->peerAddr().toIpPort().c_str(), msg.c_str());
    }

    EventLoop* loop_;
    SerialPort serialport_;
};


int main()
{
    EventLoop loop;

    mymuduo::struct_serial config1;
    config1.serialName = "ttyS1";
    config1.baudRate = 9600;
    config1.dataBits = 8;
    config1.stopBits = 1;
    config1.parityBit = mymuduo::enum_data_parityBit_N;
    testSerialPort serialport1(&loop, config1);
    serialport1.start();

    // mymuduo::struct_serial config2;
    // config2.serialName = "ttyS2";
    // config2.baudRate = 9600;
    // config2.dataBits = 8;
    // config2.stopBits = 1;
    // config2.parityBit = mymuduo::enum_data_parityBit_N;
    // testSerialPort serialport2(&loop, config2);
    // serialport2.start();

    loop.loop();
}