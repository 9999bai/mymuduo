#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
// #include "EventLoop.h"


class Buffer;
class AbstractConnection;
class AbstractConnector;
class Timestamp;
class EventLoop;


using ThreadInitCallback = std::function<void(EventLoop*)>;

using ConnectorPtr = std::shared_ptr<AbstractConnector>;
using ConnectionPtr = std::shared_ptr<AbstractConnection>;
using ConnectionCallback = std::function<void (const ConnectionPtr&)>;
using CloseCallback = std::function<void (const ConnectionPtr&)>;
using WriteCompleteCallback = std::function<void (const ConnectionPtr&)>;
using MessageCallback = std::function<void (const ConnectionPtr&, Buffer*, Timestamp)>;
using HighWaterMarkCallback = std::function<void(const ConnectionPtr&, size_t)>;

// using SerialConnectorPtr = std::shared_ptr<SerialConnector>;

using TimerCallback = std::function<void()>;

using ConnectionMap = std::unordered_map<std::string, ConnectionPtr>;

#define HIGHWATER 2*1024*1024





