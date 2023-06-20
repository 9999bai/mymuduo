#pragma once

#include <termios.h>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "Logger.h"

class EventLoop;

namespace sockets
{
    int createNonblocking(sa_family_t family);
    int createblocking_udp();
    // int  connect(int sockfd, const struct sockaddr* addr);
    void close(int sockfd);
    int getSocketError(int sockfd);

    struct sockaddr_in getLocalAddr(int sockfd);
    struct sockaddr_in getPeerAddr(int sockfd);
    bool isSelfConnect(int sockfd);

}; // namespace sockets
namespace mymuduo
{
    EventLoop *CheckLoopNotNull(EventLoop *loop);

    typedef enum{
        enum_data_res_normal,
        enum_data_res_error,
        enum_data_res_disvalid
    } enum_data_res;

    typedef enum{
        enum_data_parityBit_N,
        enum_data_parityBit_E,
        enum_data_parityBit_O,
        enum_data_parityBit_S
    } enum_data_parityBit;

    typedef struct {
        enum_data_res valid = enum_data_res_disvalid;
        std::string serialName;
        int baudRate;
        int dataBits;
        int stopBits; 
        enum_data_parityBit parityBit;
    } struct_serial;
};
