#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"


#include <errno.h>
#include <unistd.h>
#include <string.h>

// channel未添加到poller中
const int kNew = -1; //channel的成员index = -1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;


EPollPoller::EPollPoller(EventLoop* loop) 
                : Poller(loop)
                , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
                , events_(kInitEventListSize)
{
    if(epollfd_ < 0)
    {
        LOG_FATAL("epoll_create errro:%d\n", errno);
    }
    LOG_INFO("EPollPoller ctor...");
}

EPollPoller::~EPollPoller()
{
    close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    // LOG_DEBUG("func=%s fd total count=%d\n", __FUNCTION__, channels_.size());
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    
    if(numEvents > 0)
    {
        // LOG_INFO("%d events happened \n", numEvents);
        fillActionChannels(numEvents, activeChannels);
        if(numEvents == (int)events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller:poll error!");
        }
    }
    // LOG_INFO("111");
    // for(auto it=channels_.begin(); it!=channels_.end(); it++)
    // {
    //     // Channel*ch = it->second;
    //     LOG_INFO("222");
    //     LOG_INFO("channels_::---%d, %d : %d", it->first, (int)it->second->isReading(), (int)it->second->isWriting());
    // }

    return now;
}

/**
 *                  EventLoop
 *          ChannelList         Poller
 *                             ChannelMap <fd, channel*>
*/
void EPollPoller::updateChannel(Channel* channel)
{
    const int index = channel->index();
    // LOG_INFO("func=%s fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);
    
    if(index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if(index == kNew)
        {
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
        LOG_INFO("updateChannel CTL_ADD, fd=%d,events=%d,channels=%d\n", channel->fd(), channel->events(),channels_.size());
    }
    else    // channel已经在poller上注册过
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
            LOG_INFO("updateChannel CTL_DEL, fd=%d,events=%d,channels=%d\n", channel->fd(), channel->events(),channels_.size());
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
            LOG_INFO("updateChannel CTL_MOD, fd=%d,events=%d,channels=%d\n", channel->fd(), channel->events(),channels_.size());
        }
    }
}

void EPollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    int index = channel->index();
    channels_.erase(fd);
    LOG_INFO("channels_.erase  fd=%d, channels=%d", fd, channels_.size());
    
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::fillActionChannels(int numEvents, ChannelList *activeChannels) const
{
    for(int i=0; i<numEvents; i++)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop就拿到了它的poller给它返回的所有发生事件的channel列表了
    }
}

// 更新channel通道
void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    // event.data.fd = fd;
    event.data.ptr = channel;
    int fd = channel->fd();
    
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl mod/add error:%d, epollfd = %d, fd = %d\n", errno, epollfd_, fd);
        }
    }
}

