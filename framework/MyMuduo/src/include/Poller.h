#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include <vector>
#include <unordered_map>

class EventLoop;
class Channel;


// muduo库中多路事件分发器的核心IO复用模块, 抽象类
class Poller : noncopyable
{

public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;


    // epoll_wait
    virtual Timestamp poll(int timeoutMs, ChannelList * activeChannels) = 0;
    // epoll_ctl add 
    virtual void updataChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel * channel) = 0;

    bool hasChannel(Channel * channel) const;

    // 通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop *loop);

protected:
    // map的key：sockfd  value：sockfd所属的channel通道类型
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    
    EventLoop * ownerLoop_; //所属的事件循环
};

