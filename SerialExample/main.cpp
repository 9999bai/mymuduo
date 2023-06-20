#include "/usr/include/mymuduo/SerialPort.h"
#include "/usr/include/mymuduo/ThreadPool.h"
#include "/usr/include/mymuduo/Buffer.h"
#include "/usr/include/json-arm/json.h"

// #include "Helper.h"
#include <memory>
#include <semaphore.h>
#include <atomic>
#include <string>
#include <list>


class testSerialPort
{
public:
    testSerialPort(EventLoop* loop, const mymuduo::struct_serial& serialConfig)
            : serialport_(loop, serialConfig)
            , loop_(loop)
            , name_(serialConfig.serialName)
            , poolPtr_(std::make_shared<ThreadPool>("IOT_Gateway ThreadPool"))
            , timerCount_(0)

    {
        ThreadPoolInit();
        // LOG_INFO("1111");
        serialport_.setConnectionCallback(std::bind(&testSerialPort::onConnection, this, std::placeholders::_1));
        serialport_.setMessageCallback(std::bind(&testSerialPort::onMessage, this, std::placeholders::_1,std::placeholders::_2, std::placeholders::_3));
        serialport_.enableRetry();
        // LOG_INFO("2222");

        // sem_init(&sem_RD, false, 0);
        // sem_init(&sem_TD, false, 0);

        // poolPtr_->run(std::bind(&testSerialPort::sendDataTD, this));
        // poolPtr_->run(std::bind(&testSerialPort::sendDataRD, this));

        loop_->runEvery(0.5, std::bind(&testSerialPort::semPost, this));
        loop_->runEvery(2, std::bind(&testSerialPort::nothing, this));
        // LOG_INFO("3333");
    }
    ~testSerialPort() { }

    void start() { serialport_.connect(); }

private:

    void onConnection(const ConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("onConnection - conn UP: %s", conn->name().c_str());
            conn_ = conn;
            getData_EveryT_ = loop_->runEvery(0.3, std::bind(&testSerialPort::getMsg, this));
        }
        else
        {
            LOG_INFO("onConnection - conn DOWN: %s", conn->name().c_str());
            loop_->cancel(getData_EveryT_);
        }
    }

    void onMessage(const ConnectionPtr& conn, Buffer* buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        buff_.append(msg.c_str(), msg.size());
    }

    void ThreadPoolInit()
    {
        poolPtr_->setMaxQueueSize(20);                                  // 队列最大容量
        poolPtr_->start(6);//开启线程数量
        LOG_INFO("GatewayManage::ThreadPoolInit over...");
    }

    void sendDataTD()
    {
        static int count = 0;
        if(count++ > 40)
        {
            count = 0;
            std::string jsonStr = itemToJson("TD");
            conn_->send(jsonStr);
        }
        timerCount_--;
        // LOG_INFO("sendDataTD timerCount_--");
    }

    void sendDataRD()
    {
        std::string jsonStr = itemToJson("RD");
        conn_->send(jsonStr);
        // LOG_INFO("sendDataRD over");
        timerCount_--;
        // LOG_INFO("sendDataRD timerCount_--");
    }

    void getMsg()  //定时获取缓存区数据
    {
        if(buff_.readableBytes() > 0)
        {
            LOG_INFO("[%s] RECV: %s", name_.c_str(), buff_.retrieveAllAsString().c_str());
        }
    }

    void semPost()
    {
        if(timerCount_ != 0)
        {
            LOG_INFO("Timer timerCount_..................");
            return;
        }
        // LOG_INFO("Timer timerCount_ = 2");
        timerCount_ = 2;
        poolPtr_->run(std::bind(&testSerialPort::sendDataTD, this));
        poolPtr_->run(std::bind(&testSerialPort::sendDataRD, this));
    }

    void nothing()
    {

    }

    std::string itemToJson(const std::string& title)
    {
        Json::Value root;
        Json::FastWriter writer;
        Json::Value content;
        Json::Value node;

        node["gateway_id"] = 1;
        node["device_id"] = 2;
        node["device_addr"] = "3";
        node["param_id"] = 4;
        node["param_name"] = "5";
        node["value"] = "6";
        content[0] = node;

        root[title] = content;
        std::string jsonStr(writer.write(root).c_str());
        // LOG_INFO("json : %s --- json.size = %d ", jsonStr.c_str() ,jsonStr.size());
        return jsonStr;
    }


    EventLoop* loop_;
    std::string name_;

    std::shared_ptr<ThreadPool> poolPtr_;
    std::atomic<int8_t> timerCount_;
    // sem_t sem_RD;
    // sem_t sem_TD;

    Buffer buff_;
    TimerId getData_EveryT_;
    ConnectionPtr conn_;
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

    mymuduo::struct_serial config2;
    config2.serialName = "ttyS2";
    config2.baudRate = 9600;
    config2.dataBits = 8;
    config2.stopBits = 1;
    config2.parityBit = mymuduo::enum_data_parityBit_N;
    testSerialPort serialport2(&loop, config2);
    serialport2.start();

    loop.loop();
}