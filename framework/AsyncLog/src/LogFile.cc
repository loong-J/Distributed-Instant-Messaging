// LogFile.cc
#include "LogFile.h"
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <ctime>



// --- AppendFile 内部类（Pimpl实现） ---
/**
 * @brief 底层文件写入封装类（LogFile的内部实现类）
 * @details 封装标准C库的文件操作，提供高效的日志写入接口，
 *          核心优化：使用64KB缓冲区减少系统调用，支持无锁写入（Linux特有）。
 *          注意：该类非线程安全，由上层LogFile保证线程安全。
 */
class AppendFile : NonCopyable {
public:
    /**
     * @brief 构造函数：打开文件并初始化缓冲区
     * @param filename 要打开的日志文件名
     * @details 
     *  1. 打开模式"ae"：a=追加写入，e=O_CLOEXEC（执行exec时关闭文件描述符）
     *  2. setbuffer设置自定义缓冲区，替代默认的stdio缓冲区，提升写入性能
     */
    explicit AppendFile(const std::string& filename)
        : fp_(::fopen(filename.c_str(), "ae")), writtenBytes_(0) {
        assert(fp_); 
        // 设置文件缓冲区：使用自定义的64KB缓冲区，减少write系统调用次数
        ::setbuffer(fp_, buffer_, sizeof(buffer_));
    }

    /**
     * @brief 析构函数：关闭文件句柄
     * @details 自动关闭文件，避免文件描述符泄漏
     */
    ~AppendFile() { 
        if (fp_) ::fclose(fp_); 
    }

    /**
     * @brief 追加日志内容到文件（核心写入接口）
     * @param logline 待写入的日志内容指针
     * @param len 日志内容长度（字节）
     * @details 
     *  1. 循环写入直到所有数据写完（处理部分写入的情况）
     *  2. 使用fwrite_unlocked（Linux特有）避免锁开销，非Linux系统需替换为fwrite
     *  3. 记录已写入总字节数，用于判断文件滚动条件
     */
    void append(const char* logline, const size_t len) {
        size_t written = 0; // 已写入字节数
        // 循环写入：处理单次fwrite返回小于期望长度的情况（如缓冲区满、信号中断）
        while (written != len) {
            size_t remain = len - written; // 剩余待写入字节数
            // 注意：fwrite_unlocked 是 Linux 特有，非 Linux 请用 fwrite
            // 无锁写入：减少stdio内部锁的开销，提升并发性能
            size_t n = ::fwrite_unlocked(logline + written, 1, remain, fp_);
            
            // 处理写入异常
            if (n != remain) {
                int err = ferror(fp_); // 获取文件错误码
                if (err) {
                    // 输出错误信息到标准错误流，不中断程序
                    fprintf(stderr, "AppendFile::append() failed %s\n", strerror(err));
                    break;
                }
            }
            written += n; // 更新已写入字节数
        }
        writtenBytes_ += written; // 累计总写入字节数
    }

    /**
     * @brief 强制刷盘：将stdio缓冲区数据写入内核，最终刷到磁盘
     * @details 调用fflush，确保数据不滞留在内核缓冲区，降低数据丢失风险
     */
    void flush() { 
        ::fflush(fp_); 
    }

    /**
     * @brief 获取已写入文件的总字节数
     * @return off_t 总字节数（兼容大文件，64位）
     */
    off_t writtenBytes() const { 
        return writtenBytes_; 
    }

private:
    FILE* fp_;               // 文件句柄（C标准库）
    char buffer_[64 * 1024]; // 64KB自定义缓冲区（减少系统调用）
    off_t writtenBytes_;     // 累计写入字节数（用于判断文件大小是否达到滚动阈值）
};


// --- LogFile 实现 ---

LogFile::LogFile(const std::string& basename, off_t rollSize, bool threadSafe, int checkEveryN)
    : basename_(basename), rollSize_(rollSize), checkEveryN_(checkEveryN), count_(0),
      mutex_(threadSafe ? new std::mutex : nullptr), startOfPeriod_(0), lastRoll_(0), lastFlush_(0) {
    rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char* logline, int len) {
    if (mutex_) {
        std::lock_guard<std::mutex> lock(*mutex_);
        append_unlocked(logline, len);
    } else {
        append_unlocked(logline, len);
    }
}

void LogFile::flush() {
    if (mutex_) {
        std::lock_guard<std::mutex> lock(*mutex_);
        file_->flush();
    } else {
        file_->flush();
    }
}

void LogFile::append_unlocked(const char* logline, int len) {
    file_->append(logline, len);
    // 超过日志最大字节
    if (file_->writtenBytes() > rollSize_) {
        rollFile();
    } else {
        ++count_;
        // 每写入checkEveryN_检查是否需要文件滚动
        if (count_ >= checkEveryN_) {
            count_ = 0;
            time_t now = ::time(NULL);
            // 判断日志是否跨天
            time_t thisPeriod = now / (60 * 60 * 24) * (60 * 60 * 24);
            if (thisPeriod != startOfPeriod_)
            {
                rollFile();
            } else if (now - lastFlush_ > 3) // 间隔大于三秒刷盘
            {
                lastFlush_ = now;
                file_->flush();
            }
        }
    }
}

bool LogFile::rollFile() {
    time_t now = 0;
    std::string filename = getLogFileName(basename_, &now);
    //  当前日期零点
    time_t start = now / (60 * 60 * 24) * (60 * 60 * 24);

    if (now > lastRoll_) {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(new AppendFile(filename));
        return true;
    }
    return false;
}

std::string LogFile::getLogFileName(const std::string& basename, time_t* now) {
    std::string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tm;
    *now = time(NULL);
    localtime_r(now, &tm);
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;

    char hostname[256];
    if (gethostname(hostname, sizeof hostname) == 0) {
        hostname[sizeof(hostname)-1] = '\0';
        filename += hostname;
    } else {
        filename += "unknownhost";
    }

    char pidbuf[32];
    snprintf(pidbuf, sizeof pidbuf, ".%d", ::getpid());
    filename += pidbuf;
    filename += ".log";
    return filename;
}

