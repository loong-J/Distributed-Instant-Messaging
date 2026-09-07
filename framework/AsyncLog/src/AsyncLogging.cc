// AsyncLogging.cc
#include "AsyncLogging.h"
#include "LogFile.h"
#include <chrono>
#include <cassert>
#include <iostream>
#include <string.h>


AsyncLogging::AsyncLogging(const std::string& basename, int flushInterval)
    : flushInterval_(flushInterval), running_(false), basename_(basename),
      currentBuffer_(new Buffer), nextBuffer_(new Buffer) {
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

AsyncLogging::~AsyncLogging() {
    if (running_) stop();
}

void AsyncLogging::start() {
    running_ = true;
    thread_ = std::thread([this]() { threadFunc(); });
}

void AsyncLogging::stop() {
    running_ = false;
    cond_.notify_all();
    if (thread_.joinable()) thread_.join();
}

void AsyncLogging::append(const char* logline, int len) {
    std::lock_guard<std::mutex> lock(mutex_);
    // 情况1：当前缓冲区有足够空间 → 直接追加日志
    if (currentBuffer_->avail() > len) 
    {
        currentBuffer_->append(logline, len);
    } 
     // 情况2：当前缓冲区满 → 切换缓冲区并唤醒后台线程
    else 
    {
         // 1. 将写满的当前缓冲区移入待写入队列
        buffers_.push_back(std::move(currentBuffer_));
        if (nextBuffer_) {
            // 2. 切换预备缓冲区为新的当前缓冲区
            currentBuffer_ = std::move(nextBuffer_);
        } else {
            // 在极端下：如果前端写入速度 > 后端刷盘速度，后端还没来得及归还 buffer，此时 nextBuffer_ 为空。前端只能被迫 new 一个新 Buffer。
            currentBuffer_.reset(new Buffer); // 极少发生
        }
        // 3. 将当前日志写入新的当前缓冲区
        currentBuffer_->append(logline, len);
        cond_.notify_one();
    }
}

void AsyncLogging::threadFunc() {
    assert(running_ == true);
    
    // 初始化 Output (LogFile)
    // 单个日志文件最大50MB，无需线程安全
    LogFile output(basename_, 50 * 1024 * 1024, false);

    // 初始化两个新缓冲区，用于复用（替代nextBuffer_/currentBuffer_）
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    // 待写入的缓冲区列表
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while (running_) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 条件等待：
        // - 如果buffers_为空（无待刷盘数据），等待flushInterval_秒
        // - 被唤醒的两种情况：
        //      - 超时（3秒）/
        //      - 前端缓冲区满 ===> buffers_有数据（append调用notify_one）
        if (buffers_.empty()) {
            cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
        }

        // 核心操作：将当前缓冲区移入待写入队列（即使为空，也确保刷盘
        buffers_.push_back(std::move(currentBuffer_));
        // 把后端的空闲块 newBuffer1 给前端
        currentBuffer_ = std::move(newBuffer1);
        // 把前端满载的 buffers_ 和后端空的 buffersToWrite 交换
        buffersToWrite.swap(buffers_);
        // 如果前端连备用块 nextBuffer_ 都用完了，把 newBuffer2 给它
        if (!nextBuffer_) {
            nextBuffer_ = std::move(newBuffer2);
        }
    }
    //解锁

        assert(!buffersToWrite.empty());

        // 内存堆积，直接丢弃掉中间的缓冲区，只保留开头（案发初期）的日志
        if (buffersToWrite.size() > 25) {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages... %zd buffers\n", buffersToWrite.size() - 2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }

        for (const auto& buffer : buffersToWrite) {
            output.append(buffer->data(), buffer->length());
        }
        
        /**
         * 内存伸缩：如果发生突发流量（Burst），
         * buffersToWrite 可能会瞬间增长到 100 个 Buffer（400MB）。
         * 流量过去后，这个 resize(2) 会销毁多余的 98 个 Buffer，释放内存。
         * 这让日志系统不会长期霸占高峰期申请的内存。
         */
        if (buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }
        // 2. 回收 newBuffer1
        if (!newBuffer1) {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }
        // 3. 回收 newBuffer2
        if (!newBuffer2) {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}

