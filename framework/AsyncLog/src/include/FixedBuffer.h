#pragma once

#include "NonCopyable.h"
#include <cstring>

// 模板类    核心内存缓冲组件
template <int SIZE>
class FixedBuffer : NonCopyable
{
public:
    FixedBuffer() : cur_(data_) {}
    ~FixedBuffer() = default;

    // 向缓冲区追加数据
    void append(const char *buf, size_t len)
    {
        if (avail() > static_cast<int>(len))
        {
            std::memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char *data() const { return data_; }
    int length() const { return static_cast<int>(cur_ - data_); }
    char *current() { return cur_; }

    // 返回剩余空间
    int avail() const { return static_cast<int>(end() - cur_); }
    void add(size_t len) { cur_ += len; }

    void reset() { cur_ = data_; }
    void bzero() { std::memset(data_, 0, sizeof(data_)); }

private:
    const char *end() const { return data_ + sizeof(data_); }

    char data_[SIZE];   // 实际存储数据的数组
    char *cur_;         // 指向当前写入位置的指针
};