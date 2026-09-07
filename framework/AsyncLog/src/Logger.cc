#include "Logger.h"
#include <thread>
#include <ctime>
#include <sys/time.h> 
#include <errno.h>
#include <cstdlib> // for abort

// 初始化全局日志级别 (默认为 INFO)
Logger::LogLevel g_logLevel = Logger::INFO;

void Logger::setLogLevel(Logger::LogLevel level) {
    g_logLevel = level;
}

// 默认输出
void defaultOutput(const char* msg, int len) {
    size_t n = fwrite(msg, 1, len, stdout);
    (void)n;
}
void defaultFlush() {
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

void Logger::setOutput(OutputFunc out) { g_output = out; }
void Logger::setFlush(FlushFunc flush) { g_flush = flush; }

// 辅助类：日志等级转字符串
const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

// Impl 构造函数
Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
    : level_(level), line_(line), basename_(file) {
    
    // 1. 打印时间
    formatTime();
    
    // 2. 打印线程ID
    // 建议：如果你有 CurrentThread::tid() 缓存，这里替换掉 hash 性能会更好
    auto tid = std::this_thread::get_id();
    stream_ << " " << std::hash<std::thread::id>{}(tid) << " ";
    
    // 3. 打印日志等级 (补充完整)
    stream_ << LogLevelName[level];
    
    // 4. 如果有错误码，打印错误描述
    if (savedErrno != 0) {
        stream_ << strerror(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

void Logger::Impl::formatTime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t t = tv.tv_sec;
    struct tm tm_time;
    ::localtime_r(&t, &tm_time);

    char buf[64];
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06ld",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, tv.tv_usec);
    stream_ << buf;
}

void Logger::Impl::finish() {
    stream_ << " - " << basename_.data_ << ":" << line_ << "\n";
}

// 构造函数重载实现
Logger::Logger(SourceFile file, int line) : impl_(INFO, 0, file, line) {}

Logger::Logger(SourceFile file, int line, LogLevel level) 
    : impl_(level, 0, file, line) {}

// 新增：带函数名的构造函数 (用于 TRACE/DEBUG)
Logger::Logger(SourceFile file, int line, LogLevel level, const char* func) 
    : impl_(level, 0, file, line) {
    impl_.stream_ << func << "() ";
}

// 析构函数 (核心逻辑补充)
Logger::~Logger() {
    impl_.finish();
    
    // 获取 buffer
    const LogStream::Buffer& buf(stream().buffer());
    
    // 输出
    g_output(buf.data(), buf.length());
    
    // 针对致命错误的处理
    if (impl_.level_ == FATAL) {
        g_flush(); // 强制刷盘
        abort();   // 终止程序，生成 Core Dump
    }
}