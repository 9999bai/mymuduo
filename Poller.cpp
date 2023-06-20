#include "Poller.h"
#include "Channel.h"
#include "Logger.h"

Poller::Poller(EventLoop *loop) : ownerLoop_(loop)
{
    LOG_INFO("Poller ctor...");
}

Poller::~Poller()
{

}

bool Poller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}
