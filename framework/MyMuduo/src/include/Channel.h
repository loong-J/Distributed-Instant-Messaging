#include "noncopyable.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <memory>
#include <functional>
#include "Timestamp.h"
/**
 * Channel 封装了socketfd
 * - 封装了socketfd 感兴趣的事件 event，如读事件EPOLLIN 、 写事件EPOLLOUT
 * - 绑定了Poller返回的具体事件，Epollwait()返回具体事件
 * - channel通道里面能够获知fd最终发生的具体的事件revents，所以它负责调用具体事件的回调操作,
 *      EventLoop 发现某个 FD 就绪时，会调用该 FD 对应的 Channel 对象中的回调函数
 */

class EventLoop; // 前向声明，适用场景：声明类的指针 / 引用、声明涉及该类的函数（仅声明），核心价值是解决循环依赖、加快编译速度。

class Channel : noncopyable
{

public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime); // Poller返回发生的具体事件 ，处理事件

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    void tie(const std::shared_ptr<void>&); // channel还在执行回调操作时，防止当channel被remove手动删除掉

    int fd() const{ return fd_; }
    int events() const{ return events_; }
    void set_revents(int revt){ revents_ = revt; }

    // 设置fd的感兴趣事件
    void enableReading(){ events_ |= KReadEvent; update(); }
    void enableWriting(){ events_ |= KWriteEvent; update(); }
    void disableReading(){ events_ &= ~KReadEvent; update(); }
    void disableWriting(){ events_ &= ~KWriteEvent; update(); }
     void disableAll() { events_ = KNoneEvent; update(); }
    // 判断fd当前是否已存在感兴趣的事件
    bool isNoneEvent() const { return events_ == KNoneEvent; }
    bool isWriting() const { return events_ & KWriteEvent; }
    bool isReading() const { return events_ & KReadEvent; }



    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // 属于哪个EventLoop
    EventLoop * ownerLoop() { return loop_; }

    void remove();


private:

    void update(); // 更新fd感兴趣的事件 epoll_ctl, 实际在Poller里操作
    
    void handleEventWithGuard(Timestamp receiveTime);



    // 事件
    static const int KNoneEvent; // fd没有感兴趣的事件
    static const int KReadEvent; 
    static const int KWriteEvent; 

    EventLoop *loop_; // channel所属的loop, channel要注册在poller中
    const int fd_;
    int events_; // 注册fd感兴趣的事件
    int revents_; // poller返回的具体就绪事件

    int index_; // 标识channel在Poller中的状态


    std::weak_ptr<void> tie_; //检测对象生命周期
    bool tied_;

    // 当前fd的回调函数
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};