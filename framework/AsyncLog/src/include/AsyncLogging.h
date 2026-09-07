// AsyncLogging.h
#pragma once
#include "NonCopyable.h"
#include "FixedBuffer.h"
#include "LogStream.h"

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

/**
 * @brief 异步日志核心类
 * @details 采用经典的「双缓冲区+后台线程」设计模式，实现日志写入与业务线程解耦：
 *          1. 前端（业务线程）：将日志写入内存缓冲区，无磁盘IO阻塞
 *          2. 后端（日志线程）：定时/定量将缓冲区数据刷入磁盘
 *          核心优势：业务线程日志写入延迟极低，磁盘IO批量执行，提升整体性能
 */
class AsyncLogging : NonCopyable {
public:
    AsyncLogging(const std::string& basename, int flushInterval = 3);
    ~AsyncLogging();

    // 前端接口：添加日志
    void append(const char* logline, int len);

    /**
    * @brief 启动异步日志器
    * @details 创建并启动后台日志线程，开始监听缓冲区数据
    */
    void start();
    /**
     * @brief 停止异步日志器
     * @details 设置运行状态为false，唤醒后台线程，等待线程退出
    */
    void stop();

private:
    /**
     * @brief 后台线程主函数（核心逻辑）
     * @details 循环执行：
     *          1. 等待（超时/被唤醒）
     *          2. 将待写入缓冲区批量刷入磁盘
     *          3. 回收复用缓冲区
     */
    void threadFunc();

    // 缓冲区类型定义
    using Buffer = FixedBuffer<kLargeBuffer>; // 4MB
    using BufferPtr = std::unique_ptr<Buffer>;
    using BufferVector = std::vector<BufferPtr>;

    const int flushInterval_;//  刷盘间隔（秒）
    std::atomic<bool> running_;//  运行状态
    const std::string basename_;
    
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    
    // 双缓冲机制的核心数据
    BufferPtr currentBuffer_; // 前端正在写的，当前缓冲
    BufferPtr nextBuffer_;    // 前端预备缓冲,currentBuffer_慢之后交换
    BufferVector buffers_;    // 待写入缓冲队列，前端写满的队列
};

