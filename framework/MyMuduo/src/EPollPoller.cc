#include "EPollPoller.h"
#include "Channel.h"

#include <AsyncLog/Logger.h>
#include <unistd.h>

// channel未添加到poller中
const int kNew = -1;  // channel的成员index_ = -1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;


EPollPoller::EPollPoller(EventLoop *loop) 
    : Poller(loop)
    , epollfd_(epoll_create1(EPOLL_CLOEXEC))
    , events_(KInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_ERROR << Fmt("epoll_create error:%d \n", errno);
    }
}
EPollPoller::~EPollPoller()
{
    close(epollfd_);
}


// 重写基类Poller的抽象方法
    // epoll_wait
Timestamp EPollPoller::poll(int timeoutMs, ChannelList * activeChannels)
{
    LOG_DEBUG << " func= " << __FUNCTION__ << Fmt("fd total count:%lu" , channels_.size());

    int numEvents = epoll_wait(epollfd_, &*events_.begin(), events_.size(), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents > 0)
    {
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2); // 扩容
        }
    }
    else if(numEvents == 0)
    {
        //  超时 ，在规定的超时时间内没有事件就绪
         LOG_DEBUG <<  __FUNCTION__ <<" timeout!";
    }
    else
    {
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
        
            LOG_ERROR << "EPollPoller::poll() err!";
        }
    }
    return now;

}



void EPollPoller::fillActiveChannels(int numEvents, ChannelList * activeChannels) const
{

    for (int i=0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events); // 设置poller返回的具体事件
        activeChannels->push_back(channel); // EventLoop就拿到了它的poller给它返回的所有发生事件的channel列表了
    }


}
// epoll_ctl add 
void EPollPoller::updataChannel(Channel *channel)
{
    const int index = channel->index();
    // channel未在poller上注册过了
    if(index == kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // channel未在poller上注册过了
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            // 修改对应的事件
            update(EPOLL_CTL_MOD, channel);
        }
    }

    
}
void EPollPoller::removeChannel(Channel * channel)
{
    int fd = channel->fd();
    channels_.erase(fd);
    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::update(int opreation, Channel* channel)
{
    epoll_event event;
    bzero(&event,sizeof event);

    int fd = channel->fd();

    event.events = channel->events();
    // event.data是一个联合体
    event.data.ptr = channel;

    if(::epoll_ctl(epollfd_, opreation, fd, &event) < 0)
    {
        if(opreation == EPOLL_CTL_DEL)
        {
            LOG_ERROR << "epoll_ctl del error: " << errno;
        }
        else
        {
            LOG_FATAL << "epoll_ctl add/mod error: " << errno;
        }

    }

}
