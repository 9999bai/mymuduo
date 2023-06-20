#include "AbstractConnector.h"


const int AbstractConnector::kMaxRetryDelayMs;
const int AbstractConnector::kInitRetryDelayMs;

int AbstractConnector::BaudRateArr_[8] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200 };
int AbstractConnector::SpeedArr_[8] = { 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200 };

AbstractConnector::AbstractConnector(EventLoop* loop, const std::string& name)
                : loop_(loop)
                , name_(name)
                , state_(kDisconnected)
                , connect_(false)
                , retryDelayMs_(kInitRetryDelayMs)
{
    // LOG_INFO("AbstractConnector ctor...");
}

AbstractConnector::AbstractConnector(EventLoop* loop, const InetAddress& serverAddr)
                : loop_(loop)
                , serverAddr_(serverAddr)
                , state_(kDisconnected)
                , connect_(false)
                , retryDelayMs_(kInitRetryDelayMs)
{
    // printf("AbstractConnector::AbstractConnector---------------2222-------------------%s", serverAddr.toIpPort().c_str());
}

AbstractConnector::~AbstractConnector()
{

}

void AbstractConnector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&AbstractConnector::startInLoop, this));
}

void AbstractConnector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void AbstractConnector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&AbstractConnector::stopInLoop, this));
}

void AbstractConnector::startInLoop()
{
    loop_->assertInLoopThread();
    if(connect_)
    {
        connect();
    }
    else
    {
        LOG_INFO(" error -- do not connect...");
    }
}

void AbstractConnector::stopInLoop()
{
    loop_->assertInLoopThread();
    if(state_ == kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void AbstractConnector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(kDisconnected);
    if(connect_)
    {
        LOG_INFO("AbstractConnector::retry connecting to %s\n", name_.c_str());
        loop_->runAfter(retryDelayMs_/1000.0, std::bind(&AbstractConnector::startInLoop, this));
        retryDelayMs_ = std::min(retryDelayMs_*2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_INFO("do not connect");
    }
}

int AbstractConnector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    loop_->queueInLoop(std::bind(&AbstractConnector::resetChannel, this));
    return sockfd;
}

void AbstractConnector::resetChannel()
{
    channel_.reset();
}