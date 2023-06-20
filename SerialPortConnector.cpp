#include "SerialPortConnector.h"
#include "Channel.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <strings.h>
#include <fcntl.h>

SerialPortConnector::SerialPortConnector(EventLoop* loop, const mymuduo::struct_serial& config)
                    : AbstractConnector(loop, config.serialName)
                    , serialConfig_(config)
{
    LOG_INFO("SerialPortConnector ctor...");
}

SerialPortConnector::~SerialPortConnector()
{

}

void SerialPortConnector::handleWrite()
{
    if(state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        setState(kConnected);
        if(connect_ && newConnectionCallback_)
        {
            newConnectionCallback_(sockfd);
        }
        else
        {
            sockets::close(sockfd);
        }
    }
}

void SerialPortConnector::handleError()
{
    if(state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_INFO("%s::handleError errno = %d\n",name_.c_str() , err);
        retry(sockfd);
    }
}

void SerialPortConnector::connect()
{
    if(serialConfig_.serialName.empty())
    {
        LOG_FATAL("InitSerialPort serialName is empty");
    }
    std::string serialname = "/dev/" + serialConfig_.serialName;
    int fd = ::open(serialname.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    int saveErrno = (fd != -1) ? 0 : errno;
    LOG_INFO("open()  fd = %d\n", fd);

    switch (saveErrno)
    {
    case 0:
        if(setSerialPort(fd))
        {
            LOG_INFO("serial:[%s] open success [fd=] = %d\n", serialConfig_.serialName.c_str(), fd);
        }
        else
        {
            LOG_FATAL("serial:[%s] open failed [fd=] = %d\n", serialConfig_.serialName.c_str(), fd);
        }
        connecting(fd);
    break;
    default:
        sockets::close(fd);
        LOG_FATAL("SerialConnector [%s] open failed, errno = %d\n", serialConfig_.serialName.c_str(), saveErrno);
    break;
    }
}

void SerialPortConnector::connecting(int sockfd)
{
    setState(kConnecting);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallabck(std::bind(&SerialPortConnector::handleWrite, this));
    channel_->setErrorCallback(std::bind(&SerialPortConnector::handleError, this));
    channel_->enableWriting();
}

bool SerialPortConnector::setSerialPort(int sockfd)
{
    struct termios opt;
    bzero(&opt, sizeof opt);
    if(0 != tcgetattr(sockfd, &opt))
    {
        LOG_INFO("InitSerialPort tcgetattr error");
        return false;
    }

    for(int i=0; i<sizeof(BaudRateArr_) / sizeof(int); i++)
    {
        if(serialConfig_.baudRate == SpeedArr_[i])
        {
            tcflush(sockfd, TCIOFLUSH);
            cfsetispeed(&opt, BaudRateArr_[i]);
            cfsetospeed(&opt, BaudRateArr_[i]);
            break;
        }
        if(i == sizeof(SpeedArr_)/sizeof(int))
        {
            return false;
        }
    }

    opt.c_cflag |= CLOCAL;  //控制模式, 保证程序不会成为端口的占有者
    opt.c_cflag |= CREAD;   //控制模式, 使能端口读取输入的数据

    // 设置数据位
    opt.c_cflag &= ~CSIZE;
     switch (serialConfig_.dataBits)
    {
    case 5:
        opt.c_cflag |= CS5;
        break; //5位数据位
    case 6:
        opt.c_cflag |= CS6;
        break; //6位数据位
    case 7:
        opt.c_cflag |= CS7;
        break; //7位数据位
    case 8:
        opt.c_cflag |= CS8;
        break; //8位数据位
    default:
        LOG_FATAL("InitSerialPort dataBits error");
    }

    // 设置停止位
    switch (serialConfig_.stopBits)
    {
    case 1:
        opt.c_cflag &= ~CSTOPB;
        break; //1位停止位
    case 2:
        opt.c_cflag |= CSTOPB;
        break; //2位停止位
    default:
        LOG_FATAL("InitSerialPort stopBits error");
    }

    // 设置奇偶校验位
    switch (serialConfig_.parityBit)
    {
    case mymuduo::enum_data_parityBit_N:
        opt.c_cflag &= ~PARENB; // 关闭c_cflag中的校验位使能标志PARENB）
        opt.c_iflag &= ~INPCK;  // 关闭输入奇偶检测
        break;
    case mymuduo::enum_data_parityBit_O:
        opt.c_cflag |= (PARODD | PARENB); //激活c_cflag中的校验位使能标志PARENB，同时进行奇校验
        opt.c_iflag |= INPCK;             // 开启输入奇偶检测
        break;
    case mymuduo::enum_data_parityBit_E:
        opt.c_cflag |= PARENB;  //激活c_cflag中的校验位使能标志PARENB
        opt.c_cflag &= ~PARODD; // 使用偶校验
        opt.c_iflag |= INPCK;   // 开启输入奇偶检测
        break;
    case mymuduo::enum_data_parityBit_S:
        opt.c_cflag &= ~PARENB; // 关闭c_cflag中的校验位使能标志PARENB）
        opt.c_cflag &= ~CSTOPB; // 设置停止位位一位
        break;
    default:
        LOG_FATAL("InitSerialPort parityBit error");
    }

    opt.c_iflag &= ~ICRNL; //0X0D被转换为0X0A的问题
    opt.c_iflag &= ~(IXON);//0x11、0x13  清bit位 关闭流控制符
    opt.c_oflag &= ~OPOST;                          // 设置为原始输出模式
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 设置为原始输入模式
    /*所谓标准输入模式是指输入是以行为单位的，可以这样理解，输入的数据最开始存储在一个缓冲区里面（但并未真正发送出去），
    可以使用Backspace或者Delete键来删除输入的字符，从而达到修改字符的目的，当按下回车键时，输入才真正的发送出去，这样终端程序才能接收到。通常情况下我们都是使用的是原始输入模式，也就是说输入的数据并不组成行。在标准输入模式下，系统每次返回的是一行数据，在原始输入模式下，系统又是怎样返回数据的呢？如果读一次就返回一个字节，那么系统开销就会很大，但在读数据的时候，我们也并不知道一次要读多少字节的数据，
    解决办法是使用c_cc数组中的VMIN和VTIME，如果已经读到了VMIN个字节的数据或者已经超过VTIME时间，系统立即返回。*/

    opt.c_cc[VTIME] = 1;
    opt.c_cc[VMIN] = 1;

    /*刷新串口数据
    TCIFLUSH:刷新收到的数据但是不读 
    TCOFLUSH:刷新写入的数据但是不传送 
    TCIOFLUSH:同时刷新收到的数据但是不读，并且刷新写入的数据但是不传送。 */
    tcflush(sockfd, TCIFLUSH);

    // 激活配置
    if (0 != tcsetattr(sockfd, TCSANOW, &opt))
    {
        LOG_FATAL("InitSerialPort tecsetattr() failed");
    }
    return true;
}




// //
// #include "SerialConnector.h"

// #include "Logger.h"
// #include "Channel.h"
// #include "EventLoop.h"

// #include <errno.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <sys/stat.h>
// #include <strings.h>
// #include <fcntl.h>

// int BaudRateArr_[8] = { B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200 };
// int SpeedArr_[8] = { 115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200 };

// const int SerialConnector::kMaxRetryDelayMs;

// SerialConnector::SerialConnector(EventLoop* loop, const mymuduo::struct_serial& serialConfig)
//                 : loop_(loop)
//                 , serialConfig_(serialConfig)
//                 , connect_(false)
//                 , state_(kDisconnected)
//                 , retryDelayMs_(kInitRetryDelayMs)
// {
//     LOG_INFO("serialConf : %s--%d--%d--%d--%d \n", serialConfig_.serialName.c_str(), 
//                             serialConfig_.baudRate, serialConfig_.dataBits,
//                             serialConfig_.stopBits, serialConfig_.parityBit);
// }

// SerialConnector::~SerialConnector()
// {
//     LOG_INFO("SerialConnector ---dtor---");
// }

// void SerialConnector::start()
// {
//     LOG_INFO("SerialConnector::start...");
//     connect_ = true;
//     loop_->runInLoop(std::bind(&SerialConnector::startInLoop, this));
// }

// void SerialConnector::restart()
// {
//     loop_->assertInLoopThread();
//     setState(kDisconnected);
//     retryDelayMs_ = kInitRetryDelayMs;
//     connect_ = true;
//     startInLoop();
// }

// void SerialConnector::stop()
// {
//     connect_ = false;
//     loop_->queueInLoop(std::bind(&SerialConnector::stopInLoop, this));
// }

// void SerialConnector::startInLoop()
// {
//     loop_->assertInLoopThread();
//     if(connect_)
//     {
//         open();
//     }
//     else
//     {
//         LOG_INFO("do not connect");
//     }
// }

// void SerialConnector::stopInLoop()
// {
//     loop_->assertInLoopThread();
//     if(state_ == kConnecting)
//     {
//         setState(kDisconnected);
//         int sockfd = removeAndResetChannel();
//         retry(sockfd);
//     }
// }

// bool SerialConnector::setSerialPort(int sockfd)
// {
//     struct termios opt;
//     bzero(&opt, sizeof opt);
//     if(0 != tcgetattr(sockfd, &opt))
//     {
//         LOG_INFO("InitSerialPort tcgetattr error");
//         return false;
//     }

//     for(int i=0; i<sizeof(BaudRateArr_) / sizeof(int); i++)
//     {
//         if(serialConfig_.baudRate == SpeedArr_[i])
//         {
//             tcflush(sockfd, TCIOFLUSH);
//             cfsetispeed(&opt, BaudRateArr_[i]);
//             cfsetospeed(&opt, BaudRateArr_[i]);
//             break;
//         }
//         if(i == sizeof(SpeedArr_)/sizeof(int))
//         {
//             return false;
//         }
//     }

//     opt.c_cflag |= CLOCAL;  //控制模式, 保证程序不会成为端口的占有者
//     opt.c_cflag |= CREAD;   //控制模式, 使能端口读取输入的数据

//     // 设置数据位
//     opt.c_cflag &= ~CSIZE;
//      switch (serialConfig_.dataBits)
//     {
//     case 5:
//         opt.c_cflag |= CS5;
//         break; //5位数据位
//     case 6:
//         opt.c_cflag |= CS6;
//         break; //6位数据位
//     case 7:
//         opt.c_cflag |= CS7;
//         break; //7位数据位
//     case 8:
//         opt.c_cflag |= CS8;
//         break; //8位数据位
//     default:
//         LOG_FATAL("InitSerialPort dataBits error");
//     }

//     // 设置停止位
//     switch (serialConfig_.stopBits)
//     {
//     case 1:
//         opt.c_cflag &= ~CSTOPB;
//         break; //1位停止位
//     case 2:
//         opt.c_cflag |= CSTOPB;
//         break; //2位停止位
//     default:
//         LOG_FATAL("InitSerialPort stopBits error");
//     }

//     // 设置奇偶校验位
//     switch (serialConfig_.parityBit)
//     {
//     case mymuduo::enum_data_parityBit_N:
//         opt.c_cflag &= ~PARENB; // 关闭c_cflag中的校验位使能标志PARENB）
//         opt.c_iflag &= ~INPCK;  // 关闭输入奇偶检测
//         break;
//     case mymuduo::enum_data_parityBit_O:
//         opt.c_cflag |= (PARODD | PARENB); //激活c_cflag中的校验位使能标志PARENB，同时进行奇校验
//         opt.c_iflag |= INPCK;             // 开启输入奇偶检测
//         break;
//     case mymuduo::enum_data_parityBit_E:
//         opt.c_cflag |= PARENB;  //激活c_cflag中的校验位使能标志PARENB
//         opt.c_cflag &= ~PARODD; // 使用偶校验
//         opt.c_iflag |= INPCK;   // 开启输入奇偶检测
//         break;
//     case mymuduo::enum_data_parityBit_S:
//         opt.c_cflag &= ~PARENB; // 关闭c_cflag中的校验位使能标志PARENB）
//         opt.c_cflag &= ~CSTOPB; // 设置停止位位一位
//         break;
//     default:
//         LOG_FATAL("InitSerialPort parityBit error");
//     }

//     opt.c_iflag &= ~ICRNL; //0X0D被转换为0X0A的问题
//     opt.c_iflag &= ~(IXON);//0x11、0x13  清bit位 关闭流控制符
//     opt.c_oflag &= ~OPOST;                          // 设置为原始输出模式
//     opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // 设置为原始输入模式
//     /*所谓标准输入模式是指输入是以行为单位的，可以这样理解，输入的数据最开始存储在一个缓冲区里面（但并未真正发送出去），
//     可以使用Backspace或者Delete键来删除输入的字符，从而达到修改字符的目的，当按下回车键时，输入才真正的发送出去，这样终端程序才能接收到。通常情况下我们都是使用的是原始输入模式，也就是说输入的数据并不组成行。在标准输入模式下，系统每次返回的是一行数据，在原始输入模式下，系统又是怎样返回数据的呢？如果读一次就返回一个字节，那么系统开销就会很大，但在读数据的时候，我们也并不知道一次要读多少字节的数据，
//     解决办法是使用c_cc数组中的VMIN和VTIME，如果已经读到了VMIN个字节的数据或者已经超过VTIME时间，系统立即返回。*/

//     opt.c_cc[VTIME] = 1;
//     opt.c_cc[VMIN] = 1;

//     /*刷新串口数据
//     TCIFLUSH:刷新收到的数据但是不读 
//     TCOFLUSH:刷新写入的数据但是不传送 
//     TCIOFLUSH:同时刷新收到的数据但是不读，并且刷新写入的数据但是不传送。 */
//     tcflush(sockfd, TCIFLUSH);

//     // 激活配置
//     if (0 != tcsetattr(sockfd, TCSANOW, &opt))
//     {
//         LOG_FATAL("InitSerialPort tecsetattr() failed");
//     }
//     return true;
// }

// void SerialConnector::open()
// {
//     if(serialConfig_.serialName.empty())
//     {
//         LOG_FATAL("InitSerialPort serialName is empty");
//     }
//     std::string serialname = "/dev/" + serialConfig_.serialName;
//     int fd = ::open(serialname.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
//     int saveErrno = (fd != -1) ? 0 : errno;
//     LOG_INFO("open()  fd = %d\n", fd);

//     switch (saveErrno)
//     {
//     case 0:
//         if(setSerialPort(fd))
//         {
//             LOG_INFO("serial:[%s] open success [fd=] = %d\n", serialConfig_.serialName.c_str(), fd);
//         }
//         else
//         {
//             LOG_FATAL("serial:[%s] open failed [fd=] = %d\n", serialConfig_.serialName.c_str(), fd);
//         }
//         opening(fd);
//     break;
//     default:
//         sockets::close(fd);
//         LOG_FATAL("SerialConnector [%s] open failed, errno = %d\n", serialConfig_.serialName.c_str(), saveErrno);
//     break;
//     }
// }

// void SerialConnector::opening(int sockfd)
// {
//     setState(kConnecting);
//     channel_.reset(new Channel(loop_, sockfd));
//     channel_->setWriteCallabck(std::bind(&SerialConnector::handleWrite, this));
//     channel_->setErrorCallback(std::bind(&SerialConnector::handleError, this));
//     channel_->enableWriting();
// }

// void SerialConnector::handleWrite()
// {
//     if(state_ == kConnecting)
//     {
//         int sockfd = removeAndResetChannel();
//         // int err = sockets::getSocketError(sockfd);
//         // if(err)
//         // {
//         //     LOG_INFO("SerialConnector getSocketError");
//         //     LOG_ERROR("SerialConnector::handleWrite sockfd=%d, errno=%d\n", sockfd, err);
//         //     retry(sockfd);
//         // }
//         // else
//         // {
//         setState(kConnected);
//         if(connect_ && newConnectionCallback_)
//         {
//             newConnectionCallback_(sockfd);
//         }
//         else
//         {
//             sockets::close(sockfd);
//         }
//     }
// }

// void SerialConnector::handleError()
// {
//     if(state_ == kConnecting)
//     {
//         int sockfd = removeAndResetChannel();
//         int err = sockets::getSocketError(sockfd);
//         LOG_INFO("SerialConnector::handleError errno = %d\n",err);
//         retry(sockfd);
//     }
// }

// void SerialConnector::retry(int sockfd)
// {
//     sockets::close(sockfd);
//     setState(kDisconnected);
//     if(connect_)
//     {
//         LOG_INFO("SerialConnector::retry connecting to %s\n", serialConfig_.serialName.c_str());
//         // startInLoop();
//         loop_->runAfter(retryDelayMs_/1000.0, std::bind(&SerialConnector::startInLoop, shared_from_this()));
//         retryDelayMs_ = std::min(retryDelayMs_*2, kMaxRetryDelayMs);
//     }
//     else
//     {
//         LOG_INFO("do not connect");
//     }
// }

// int SerialConnector::removeAndResetChannel()
// {
//     channel_->disableAll();
//     channel_->remove();
//     int sockfd = channel_->fd();
//     loop_->queueInLoop(std::bind(&SerialConnector::resetChannel, this));
//     return sockfd;
// }

// void SerialConnector::resetChannel()
// {
//     channel_.reset();
// }
