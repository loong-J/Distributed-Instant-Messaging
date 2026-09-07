#include "EventLoop.h"  
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <AsyncLog/Logger.h>




// 防止一个线程创建多个EventLoop   thread_local
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

int creatEventfd()
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
    
        LOG_FATAL << "eventfd error: " << errno;
    
    }
    return evtfd;
}



EventLoop::EventLoop() 
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(creatEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    
    if(t_loopInThisThread)  // 一个线程创建一个loop
    {
       
        LOG_FATAL << "Another EventLoop exists in this thread  " ;

    }
    else
    {
        t_loopInThisThread = this;
    }



    //  设置wakeupfd的读事件的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 设置wakeupfd感兴趣事件类型,并加入Poller
    //完整的过程Channel_->enableReading() → update ->（Channel 内部）调用 EventLoop::updateChannel()-->Poller的方法 
    wakeupChannel_->enableReading();


}
EventLoop::~EventLoop()
{

    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    while(!quit_)
    {
        activeChannels_.clear();

        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for(Channel * channel : activeChannels_)
        {
            //Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        
        // 执行当前EventLoop事件循环需要处理的回调操作
        doPendingFunctors();


    }
    looping_ = false;
}



// 用来唤醒loop所在的线程的
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << "bytes instead of 8";
    }
}

// wake up
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
    
        LOG_ERROR << "EventLoop::handleRead() reads " << n << "bytes instead of 8";
    }
    
} 


// 退出事件循环
void EventLoop::quit()
{
    quit_ = true; // 如果在当前线程， quit_ = true之后while循环即Loop结束
    // 如果是在其它线程中，调用的quit，可能当前线程在poll阻塞中，需要唤醒，如在一个subloop(woker)中，调用了mainLoop(IO)的quit
    if(!isInLoopThread()){
        wakeup(); // 唤醒后， quit_ = true，当前线程的Loop结束
    }

}




// 在当前loop中执行Callback
void EventLoop::runInLoop(Functor Callback)
{
    // 调用者当前就在 EventLoop 所属线程（同线程）
    if(isInLoopThread())
    {
        // 在当前线程中直接执行回调
        Callback();
    }
    else // 调用者在其他线程
    {
        // 把Callback放入队列中，唤醒loop所在的线程
        queueInLoop(Callback);
    }
}
// 把Callback放入队列中，唤醒loop所在的线程，执行Callback
void EventLoop::queueInLoop(Functor Callback)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(Callback);
    }
    //callingPendingFunctors_为true: 当前loop正在执行回调，但是
    // loop中（pendingFunctors_）又有了新的回调, wakeup 避免下一轮 loop 被 poll() 阻塞,保证新任务被及时执行

    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();// 唤醒loop所在的线程
    }
}


// 执行回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const Functor & functor : functors)
    {
        // 执行当前loop需要执行的回调操作
        functor();
    }
    callingPendingFunctors_ = false;
}


//     EventLoop的方法 =》 Poller的方法         
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updataChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}
