#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
// #include "EventLoop.h"


class Buffer;
class TcpConnection;
// class UdpConnection;
class SerialConnector;
class Timestamp;
class EventLoop;


using ThreadInitCallback = std::function<void(EventLoop*)>;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>;
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>;
using MessageCallback = std::function<void (const TcpConnectionPtr&, Buffer*, Timestamp)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;

using SerialConnectorPtr = std::shared_ptr<SerialConnector>;

using TimerCallback = std::function<void()>;

using ConnectionMap = std::unordered_map<std::string, SerialConnectorPtr>;

#define HIGHWATER 2*1024*1024





