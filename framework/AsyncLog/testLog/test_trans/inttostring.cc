#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <cstring>

// ==========================================
// 1. 你的自定义实现 (查表法)
// ==========================================

// 修正后的 digits 表（去掉 9899 前缀，直接从 00 开始）
const char digits[] = 
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

template <typename T>
size_t convert(char buf[], T value) {
    T i = value;
    char *p = buf;

    do {
        int lsd = static_cast<int>(i % 100);
        i /= 100;
        *p++ = digits[lsd * 2 + 1];
        *p++ = digits[lsd * 2];
    } while (i != 0);

    if (value < 0) *p++ = '-';
    *p = '\0';
    std::reverse(buf, p);
    return p - buf;
}

// ==========================================
// 2. 基准测试工具
// ==========================================

class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end - start_).count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// 防止编译器优化掉无用的计算
// 如果转换结果不使用，聪明的编译器(-O2)可能会直接删掉整个循环
volatile size_t g_totalBytes = 0; 

int main() {
    // 准备测试数据：1000万个数字
    const int kIterations = 10000000; 
    std::vector<int> numbers;
    numbers.reserve(kIterations);
    
    // 生成混合数据：从小整数到大整数
    for (int i = 0; i < kIterations; ++i) {
        numbers.push_back(i); 
    }

    std::cout << "Benchmark running on " << kIterations << " numbers..." << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;

    // --- 测试 1: Custom Implementation ---
    {
        char buf[128];
        Timer t;
        size_t total = 0;
        for (int n : numbers) {
            total += convert(buf, n);
            // 这是一个小技巧：访问一下 buf 的首字符，防止编译器认为 convert 没副作用而优化掉
            total += buf[0]; 
        }
        g_totalBytes = total;
        std::cout << "Custom (Table Lookup) : " << t.elapsed() << " seconds" << std::endl;
    }

    // --- 测试 2: snprintf ---
    {
        char buf[128];
        Timer t;
        size_t total = 0;
        for (int n : numbers) {
            // snprintf 涉及格式化字符串解析
            total += snprintf(buf, sizeof(buf), "%d", n);
            total += buf[0];
        }
        g_totalBytes = total;
        std::cout << "snprintf (C Standard) : " << t.elapsed() << " seconds" << std::endl;
    }

    // --- 测试 3: std::to_string ---
    {
        Timer t;
        size_t total = 0;
        for (int n : numbers) {
            // std::to_string 涉及内存分配(malloc)和 std::string 构造
            std::string s = std::to_string(n);
            total += s.length();
            total += s[0];
        }
        g_totalBytes = total;
        std::cout << "std::to_string (C++11): " << t.elapsed() << " seconds" << std::endl;
    }

    // --- 测试 4: std::stringstream ---
    {
        Timer t;
        size_t total = 0;
        for (int n : numbers) {
            // stringstream 涉及流对象构造、locale检查等，最慢
            std::stringstream ss;
            ss << n;
            std::string s = ss.str(); // 这里的 .str() 也会产生拷贝
            total += s.length();
            total += s[0];
        }
        g_totalBytes = total;
        std::cout << "std::stringstream     : " << t.elapsed() << " seconds" << std::endl;
    }

    return 0;
}