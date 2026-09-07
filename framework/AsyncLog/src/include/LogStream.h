#pragma once

#include "FixedBuffer.h"
#include <string>
#include <type_traits> // 引入类型萃取工具，用于模板元编程（enable_if）
#include "Fmt.h"

/**
 * @brief 日志格式化模块核心类
 * @details 负责将各种基础数据类型（整数、浮点数、字符串等）快速转换为字符串格式，
 *          并写入底层的固定大小缓冲区，是日志系统的前端格式化核心。
 *          设计目标：极致的格式化性能，避免频繁内存分配和系统调用。
 */

// 小缓冲区大小（默认4KB），用于常规日志格式化
const int kSmallBuffer = 4000;
// 大缓冲区大小（4MB），预留扩展用
const int kLargeBuffer = 4000 * 1000;

class LogStream : NonCopyable {
public:
    // 定义LogStream使用的缓冲区类型（默认使用4KB的小缓冲区）
    using Buffer = FixedBuffer<kSmallBuffer>;

    // 重载各种基础类型的流输出操作符，实现类型到字符串的转换
    LogStream& operator<<(bool v);                // 布尔类型

    // 使用模板处理所有整型
    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, LogStream&>::type
    operator<<(T v) {
        formatInteger(v);
        return *this;
    }
    LogStream& operator<<(const void*);           // 指针类型（输出内存地址）
    LogStream& operator<<(float v);               // 浮点型
    LogStream& operator<<(double);                // 双精度浮点型
    LogStream& operator<<(char v);                // 单个字符
    LogStream& operator<<(const char* str);       // C风格字符串
    LogStream& operator<<(const std::string& v);  // C++字符串
    LogStream& operator<<(const Fmt& fmt);    // 重载 LogStream 对 Fmt 的支持
    /**
     * @brief 直接向缓冲区追加数据
     * @param data 待追加的字符数据指针
     * @param len 待追加的数据长度（字节数）
     */
    void append(const char* data, int len) { buffer_.append(data, len); }

    /**
     * @brief 获取底层缓冲区的只读引用
     * @return 供上层模块（如AsyncLogging）读取数据
     */
    const Buffer& buffer() const { return buffer_; }

    /**
     * @brief 重置缓冲区（清空数据，恢复初始状态）
     * @details 日志条目不完整/需要重新写入时调用，避免脏数据
     */
    void resetBuffer() { buffer_.reset(); }

private:
    /**
     * @brief 模板函数：格式化整数类型为字符串
     * @tparam T 整数类型（int/long/unsigned long long等）
     * @param val 待格式化的整数值
     * @details 核心优化点：
     *          1. 直接在缓冲区栈空间上转换，避免临时字符串分配
     *          2. 反向转换+字符反转，比标准库itoa/ sprintf更快
     *          3. 预分配kMaxNumericSize大小的栈空间，避免越界
     */
    template<typename T>
    void formatInteger(T);

    // 底层固定大小缓冲区，所有格式化后的日志数据都存储在此
    Buffer buffer_;

    // 整数转字符串的最大缓冲区大小（32字节）：
    // 覆盖所有整数类型（包括64位无符号数的最大字符串长度）
    static const int kMaxNumericSize = 32;
};