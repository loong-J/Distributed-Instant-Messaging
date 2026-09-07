#include "EventLoop.h"
#include "Channel.h"

#include <sys/epoll.h>
#include <AsyncLog/Logger.h>

const int Channel::KNoneEvent = 0;
const int Channel::KReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::KWriteEvent = EPOLLOUT;


Channel::Channel(EventLoop *loop, int fd) 
    : loop_(loop), fd_(fd), events_(0), revents_(0),
    index_(-1),tied_(false)
{

}

Channel::~Channel()
{

}



// channel还在执行回调操作时，防止当channel被remove手动删除掉
//一个TcpConnection新连接创建的时候, 调用，让 Channel 「绑定一个共享指针TcpConnection对象
void Channel::tie(const std::shared_ptr<void>& obj)
{
    // weakPtr 监控sharedPtr的引用计数，sharedPtr的引用计数不为0时，weakPtr  ===> sharedPtr
    tie_ = obj;
    tied_ = true;
}

// Poller返回发生的具体事件 ，处理事件
void Channel::handleEvent(Timestamp receiveTime)
{
    if(tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if(guard) // TcpConnection还在，Channel 的生命周期依赖于 TcpConnection
        {
            handleEventWithGuard(receiveTime);
        }

    }
    else
    {
         handleEventWithGuard(receiveTime);
    }


} 

void Channel::handleEventWithGuard(Timestamp receiveTime)
{

    LOG_INFO << "channel handleEvent revents: " << Fmt("%d", revents_);
    // 连接挂起
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }

    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }

    // EPOLLPRI:高优先级数据（Priority Data）」可读
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

   if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }

}


// 更新fd感兴趣的事件 epoll_ctl
void Channel::update()
{
    // 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    loop_-> updateChannel(this);
}



void Channel::remove()
{
    loop_-> removeChannel(this);
}



