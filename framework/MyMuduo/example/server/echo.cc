#include "TcpServer.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "Buffer.h"
#include <AsyncLog/Logger.h>
#include <AsyncLog/AsyncLogging.h>
#include <functional>

class EchoServer
{
public:
  EchoServer(EventLoop* loop, const InetAddress& listenAddr, int threadNum)
    : server_(loop, listenAddr, "EchoServer")
  {
    // 设置回调函数
    server_.setConnectionCallback(
        std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(
        std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置 IO 线程数
    // 0 = 单线程 (Accept 与 IO 在同一个线程)
    // N = 1个 Accept 线程 + N 个 IO 线程 (推荐设为 CPU 核数)
    server_.setThreadNum(threadNum);
  }

  void start()
  {
    server_.start();
  }

private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    // 提示：在高并发压测时，建议注释掉这里的 LOG_INFO，
    // 因为磁盘 IO 或控制台输出可能会成为瓶颈。
    LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
             << conn->localAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

  }

  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp time)
  {
    
    // 【核心逻辑】
    // 将收到的数据直接发回。
    // retrieveAll() 会把 buf 中的 readable 字节取出来，
    // Muduo 的 send(Buffer*) 实现通常非常高效，会直接交换指针或追加
    std::string s = buf->retrieveAllAsString();
    conn->send(s);

  }

  TcpServer server_;
};

// 1. 定义唯一的 AsyncLogging 实例（通常用智能指针或全局指针）
AsyncLogging* g_asyncLog = nullptr;

// 2. 定义桥接函数：Logger 产生日志后，回调这个函数
void asyncOutput(const char* msg, int len) {
    if (g_asyncLog) {
        g_asyncLog->append(msg, len);
    }
}


int main(int argc, char* argv[])
{

    // 初始化日志系统，并启动后端日志线程
    // 设置日志等级
    Logger::setLogLevel(Logger::LogLevel::DEBUG);

    // 初始化后端日志系统  名称，刷盘时间
    AsyncLogging log("MyServer", 50); 
    
    // 保存全局指针，供 asyncOutput 使用
    g_asyncLog = &log;

    // 设置 Logger 的全局输出回调
    Logger::setOutput(asyncOutput);
    
    // 启动后端日志线程
    log.start();




    LOG_INFO << "pid = " << int(getpid());

    // 默认参数
    uint16_t port = 2007;
    int threadNum = 0; // 默认为0，即单线程模式

    if (argc > 1)
    {
        port = static_cast<uint16_t>(atoi(argv[1]));
    }
    if (argc > 2)
    {
        threadNum = atoi(argv[2]);
    }

    EventLoop loop;
    InetAddress listenAddr(port);

    EchoServer server(&loop, listenAddr, threadNum);
    

    server.start();
    loop.loop();
}