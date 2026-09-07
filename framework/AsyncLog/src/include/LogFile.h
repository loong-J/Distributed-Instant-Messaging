#pragma once
#include "NonCopyable.h"
#include <string>
#include <memory>
#include <mutex>
#include <cstdio>

// AppendFile是封装底层文件写入的类
class AppendFile;

/**
 * @brief 日志文件管理核心类
 * @details 负责日志文件的创建、写入、滚动（按大小/时间）、刷盘，
 *          异步日志系统的后端文件操作层，支持多线程安全写入。
 *          核心设计：
 *          1. 按文件大小自动滚动（rollSize）
 *          2. 按时间自动滚动（按天，0点切换）
 *          3. 可选的线程安全（通过互斥锁）
 *          4. Pimpl手法隐藏底层FILE*实现，降低耦合
 */
class LogFile : NonCopyable {
public:
    /**
     * @brief 构造函数：初始化日志文件管理器
     * @param basename 日志文件基础名（如"app.log"）
     * @param rollSize 单个日志文件的最大大小（字节），超过则滚动
     * @param threadSafe 是否线程安全（默认true，启用互斥锁）
     * @param checkEveryN 每写入N次日志，检查是否需要滚动/刷盘（默认1024）
     */
    LogFile(const std::string& basename,
            off_t rollSize,
            bool threadSafe = true,
            int checkEveryN = 1024);
    

    ~LogFile();

    /**
     * @brief 追加日志内容到文件（线程安全）
     * @param logline 待写入的日志内容指针（C风格字符串）
     * @param len 日志内容长度（字节）
     */
    void append(const char* logline, int len);

    /**
     * @brief 强制刷盘：将缓冲区数据写入磁盘（线程安全）
     * @details 调用底层AppendFile的flush，确保数据不留在用户态缓冲区
     */
    void flush();

    /**
     * @brief 手动触发日志文件滚动
     * @return bool 滚动成功返回true，失败返回false
     * @details 创建新的日志文件，关闭旧文件，更新滚动时间等状态
     */
    bool rollFile();

private:
    /**
     * @brief 无锁追加日志内容（内部使用，非线程安全）
     * @param logline 待写入的日志内容指针
     * @param len 日志内容长度
     * @details 由append()加锁后调用，避免重复加锁开销
     */
    void append_unlocked(const char* logline, int len);

    /**
     * @brief 静态函数：生成带时间戳的日志文件名
     * @param basename 日志文件基础名
     * @param now 输出参数，返回当前时间戳（秒）
     * @return std::string 生成的日志文件名（如"app.log.20260127120000"）
     * @details 按时间格式化文件名，用于日志滚动时创建新文件
     */
    static std::string getLogFileName(const std::string& basename, time_t* now);

  
    /** 日志文件基础名（如"app.log"），滚动时会在其后追加时间戳 */
    const std::string basename_;
    /** 单个日志文件的最大大小（字节），超过该值触发滚动 */
    const off_t rollSize_;
    /** 每写入checkEveryN_次日志，检查一次是否需要滚动/刷盘 */
    const int checkEveryN_;
    /** 已写入日志的计数，达到checkEveryN_时重置并执行检查 */
    int count_;

 
    std::unique_ptr<std::mutex> mutex_;
    /** 日志周期起始时间（按天划分，如当天0点的时间戳），用于按时间滚动 */
    time_t startOfPeriod_;
    /** 上次滚动日志文件的时间戳 */
    time_t lastRoll_;
    /** 上次刷盘的时间戳 */
    time_t lastFlush_;
    /** 底层文件写入类指针 */
    std::unique_ptr<AppendFile> file_;

    // 补充说明：
    // off_t是系统定义的文件大小类型（通常为int64_t），兼容32/64位系统
};