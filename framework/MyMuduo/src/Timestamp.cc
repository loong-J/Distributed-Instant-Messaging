#include "Timestamp.h"

Timestamp::Timestamp() : SecondsSinceEpoch_(0){}
Timestamp::Timestamp(int64_t SecondsSinceEpoch)
    : SecondsSinceEpoch_(SecondsSinceEpoch)
    {}
Timestamp Timestamp::now()
{
    // 从 Unix 纪元时间（Unix Epoch） 到当前时刻的秒数
    return Timestamp(time(NULL));
}
std::string Timestamp::toString() const
{
    char buf[128] = {0};
    struct tm result;
    localtime_r(&SecondsSinceEpoch_, &result);

    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
        result.tm_year + 1900,
        result.tm_mon + 1,
        result.tm_mday,
        result.tm_hour,
        result.tm_min,
        result.tm_sec
    );
    return buf;
}