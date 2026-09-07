#include "LogStream.h"
#include <algorithm>   
#include <cstdio>      
#include <string.h>    
#include <limits>      
#include <cassert>     
#include <type_traits>



// --- 高性能整数转字符串：查找表优化 ---
// 核心优化：预先生成0-99的数字字符对查找表，避免逐位计算ASCII值,用空间换时间
// 每个数字占2个字符，通过下标直接访问，比计算更快
// 比如 digits[12] 对应的就是 "12"，digits[8] 对应 "08",要找数字 N，去 N * 2 的位置找。

const char digits[] = "0001020304050607080910111213141516171819"
                      "2021222324252627282930313233343536373839"
                      "4041424344454647484950515253545556575859"
                      "6061626364656667686970717273747576777879"
                      "8081828384858687888990919293949596979899";



// 十六进制数字查找表（用于指针地址格式化）
const char digitsHex[] = "0123456789ABCDEF";

/**
 * @brief 高效整数转字符串核心函数（十进制）
 * @tparam T 整数类型（int/long/unsigned long long等）
 * @param buf 输出缓冲区（栈上分配，无内存分配开销）
 * @param value 待转换的整数值
 * @return size_t 转换后的字符串长度
 * @核心优化点：
 *  1. 每次处理2位数字（除以100），减少循环次数（比逐位处理快50%+）
 *  2. 通过预生成的digits查找表直接获取字符，避免计算'0' + 余数
 *  3. 先反向写入缓冲区，最后反转，比正向插入更高效
 */
template <typename T>
size_t convert(char buf[], T value)
{
    // 后缀0，特殊处理 0
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    //unsigned 转换（防止 INT_MIN 取绝对值溢出）
    using UnsignedT = typename std::make_unsigned<T>::type;
    UnsignedT i = static_cast<UnsignedT>(value); // - 211
    if (value < 0) {
        i = 0 - i; 
    }
    char *p = buf;     // 缓冲区写指针

    // 核心循环：每次处理2位数字
    do
    {
        // 取最后两位（模100） 考虑负数取模问题 C++取模结果符号与被除数 -211
        int lsd = static_cast<int>(i % 100);    // 11 2
        i /= 100;      // 去掉最后两位           // 2   0
        // 从查找表中直接获取对应字符（先低位后高位）
        *p++ = digits[lsd * 2 + 1]; //后++ > * --> *(p++) ==> 先取指针当前指向的值，再将指针地址自增
        *p++ = digits[lsd * 2];
    } while (i != 0);  // 直到所有数字处理完毕
    // 1020 
    //去掉后缀0
    while(*(p-1) == '0'){
        p--;
    }

    // 处理负数：补充负号
    if (value < 0)
        *p++ = '-';
    *p = '\0';         // 字符串结束符（临时，反转后会覆盖）
     // \0102-
   
    std::reverse(buf, p); // == > 翻转[buf, p) 所有字符
    return p - buf;    // 返回转换后的字符串长度
}

/**
 * @brief 高效整数转十六进制字符串（用于指针地址格式化）
 * @param buf 输出缓冲区
 * @param value 待转换的无符号整数值（指针地址）
 * @return size_t 转换后的字符串长度
 * @优化思路：类似十进制转换，逐位处理16进制，通过查找表获取字符
 */
size_t convertHex(char buf[], uintptr_t value)
{
    uintptr_t i = value;  // uintptr_t是指针宽度的无符号整数类型
    char *p = buf;        // 缓冲区写指针

    do
    {
        // 取最后4位（模16）
        int lsd = static_cast<int>(i % 16);
        i /= 16;          // 去掉最后4位
        // 从十六进制查找表获取字符
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';            // 临时结束符
    std::reverse(buf, p); // 反转得到正确顺序
    return p - buf;       // 返回字符串长度
}

// --- LogStream 成员函数实现 ---

/**
 * @brief 模板函数：格式化整数类型到缓冲区
 * @tparam T 整数类型
 * @param v 待格式化的整数值
 * @details 核心逻辑：
 *  1. 先检查缓冲区剩余空间是否足够（避免越界）
 *  2. 调用convert函数将整数转字符串到缓冲区
 *  3. 更新缓冲区写指针位置
 */
template <typename T>
void LogStream::formatInteger(T v)
{
  
    if (buffer_.avail() >= kMaxNumericSize)
    {
        size_t len = convert(buffer_.current(), v);
        buffer_.add(len);
    }
}

/**
 * 在 LogStream.h 中声明了 formatInteger 函数模板,在 LogStream.cc 中实现了它。
 * 当 main.cc 包含头文件并调用 operator<<(int) 时，编译器需要生成 formatInteger<int> 的代码
 * 
 * 编译 main.cc 时，编译器看不见 LogStream.cc 里的实现代码，所以它无法生成。
 * 编译 LogStream.cc 时，编译器又不知道 main 里需要用 int 版本，所以它也没生成。
 * 最后链接时，大家都没有生成这份代码，于是报错
 */

// 强制编译器在编译 LogStream.cc 时，就把这些特定类型的代码生成出来
template void LogStream::formatInteger(int);
template void LogStream::formatInteger(unsigned int);
template void LogStream::formatInteger(long);
template void LogStream::formatInteger(unsigned long);
template void LogStream::formatInteger(long long);
template void LogStream::formatInteger(unsigned long long);




/**
 * @brief 重载bool类型的流输出操作符
 * @param v 布尔值
 * @return LogStream& 自身引用（支持链式调用，如log << true << " test";）
 */
LogStream &LogStream::operator<<(bool v)
{
    // 直接追加"1"或"0"，长度1字节
    buffer_.append(v ? "1" : "0", 1);
    return *this;
}




/**
 * @brief 重载指针类型的流输出操作符（格式化地址为十六进制）
 * 将任意类型的指针地址格式化为十六进制字符串（如0x7ffeefbff5c8），并写入日志缓冲区
 * @param p 任意类型的指针
 * @return LogStream& 自身引用
 * @details 格式：0x + 十六进制地址（如0x7ffeefbff5c8）
 */
LogStream &LogStream::operator<<(const void *p)
{
    // 将指针转换为uintptr_t（保证与指针同宽度的无符号整数）
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    
    // 检查缓冲区空间是否足够
    if (buffer_.avail() >= kMaxNumericSize)
    {
        char *buf = buffer_.current();
        buf[0] = '0';          // 十六进制前缀"0x"的第一个字符
        buf[1] = 'x';          // 十六进制前缀第二个字符
        // 转换数值到buf+2位置（跳过前缀）
        size_t len = convertHex(buf + 2, v);
        // 更新缓冲区指针（前缀2字节 + 数值长度）
        buffer_.add(len + 2);
    }
    return *this;
}

/**
 * @brief 重载double类型的流输出操作符
 * @param v 双精度浮点数
 * @return LogStream& 自身引用
 * @details 浮点数格式化使用snprintf（性能低于整数，但浮点数日志场景少）
 *          格式：%.12g 自动选择科学计数法/普通格式，保留12位有效数字
 */
LogStream &LogStream::operator<<(double v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        // 使用snprintf格式化浮点数到缓冲区
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len); // 更新缓冲区指针
    }
    return *this;
}

/**
 * @brief 重载float类型的流输出操作符
 * @param v 单精度浮点数
 * @return LogStream& 自身引用
 * @details 直接转换为double后复用double的格式化逻辑，减少重复代码
 */
LogStream &LogStream::operator<<(float v)
{
    *this << static_cast<double>(v);
    return *this;
}

/**
 * @brief 重载char类型的流输出操作符
 * @param v 单个字符
 * @return LogStream& 自身引用
 */
LogStream &LogStream::operator<<(char v)
{
    // 追加单个字符到缓冲区
    buffer_.append(&v, 1);
    return *this;
}

/**
 * @brief 重载C风格字符串的流输出操作符
 * @param str 以'\0'结尾的字符串指针
 * @return LogStream& 自身引用
 * @details 处理空指针：如果str为null，输出"(null)"而非崩溃
 */
LogStream &LogStream::operator<<(const char *str)
{
    if (str)
        // 字符串非空：追加整个字符串（通过strlen获取长度）
        buffer_.append(str, strlen(str));
    else
        // 字符串为空：追加"(null)"（6个字符）
        buffer_.append("(null)", 6);
    return *this;
}

/**
 * @brief 重载C++ std::string类型的流输出操作符
 * @param v std::string对象
 * @return LogStream& 自身引用
 */
LogStream &LogStream::operator<<(const std::string &v)
{
    // 直接追加字符串的C风格数据和长度
    buffer_.append(v.c_str(), v.size());
    return *this;
}


// 重载 LogStream 对 Fmt 的支持
LogStream& LogStream::operator<<(const Fmt& fmt) {
    buffer_.append(fmt.data(), fmt.length());
    return *this;
}