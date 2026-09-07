#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class EPollPoller : public Poller
{

public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;


    // 重写基类Poller的抽象方法
    // epoll_wait 
    virtual Timestamp poll(int timeoutMs, ChannelList * activeChannels) override;
    // epoll_ctl add 
    virtual void updataChannel(Channel *channel) override;
    virtual void removeChannel(Channel * channel) override;



private:
    static const int KInitEventListSize = 16;


    void fillActiveChannels(int numEvents, ChannelList * activeChannels) const; // 将就绪的事件集合events_填写到 activeChannels，供eventLoop使用
    void update(int opreation, Channel* channel);

    int epollfd_; //epoll文件

   
    using EventList = std::vector<epoll_event>;
    EventList events_; // epoll_wait返回的已就绪事件集合
};


