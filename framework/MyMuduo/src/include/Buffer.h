#pragma once

#include <vector>
#include <string>
#include <algorithm>

/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size

// 网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;  // (预留区)：默认 8 字节。用于在数据包前部快速添加信息（如数据包长度 Header），避免数据整体后移。
    static const size_t kInitialSize = 1024; // buffer的初始空间是1024

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}

    // 可读区的大小
    size_t readableBytes() const 
    {
        return writerIndex_ - readerIndex_;
    }
    // 可写区的大小
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    // 返回当前 Buffer 中可前置写入的字节数
    size_t prependableBytes() const
    {
        return  readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // 更新标记已经从可读区读取的数据
    // onMessage string <- Buffer
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            // 只读缓冲区的一部分数据
            readerIndex_ += len; // 应用只读取了刻度缓冲区数据的一部分，就是len，还剩下readerIndex_ += len -> writerIndex_
        }
        else   // len == readableBytes()
        // 应用层需要读取所有可读数据，比如已经完整解析了一个协议包，需要清空当前的可读数据区，准备接收下一批数据。
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        // 重置读索引和写索引到初始位置（kCheapPrepend，默认 8）
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // 应用可读取数据的长度
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 上面一句把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作
        return result;
    }

    // buffer_.size() - writerIndex_    len
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // 扩容函数
        }
    }

    // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int* saveErrno);
private:
    char* begin()
    {
        // it.operator*()
        return &*buffer_.begin();  // vector底层数组首元素的地址，也就是数组的起始地址
    }
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) // 空间的可写区域就是不够
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        { // 内部腾挪 内存拼接 能够满足可写的数据大小
            
            size_t readalbe = readableBytes();
            // 将「源区间」的元素复制到「目标区间」的起始位置，是按顺序的逐元素拷贝
            // 将 Readable 数据移动到数组头部，重置索引，从而合零为整
            std::copy(begin() + readerIndex_, 
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
    
            readerIndex_ = kCheapPrepend; 
            writerIndex_ = readerIndex_ + readalbe;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};