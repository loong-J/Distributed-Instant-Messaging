#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);


Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}


Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach(); // thread类提供的设置分离线程的方法
    }
}


void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);


    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();
        sem_post(&sem); //  对应 V 操作 +1
        func_();
    }));

    // 保证新线程启动后，tid_ 已经被正确赋值，避免主线程后续使用 tid_ 时出现未初始化的问题。
    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}


