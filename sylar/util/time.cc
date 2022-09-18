#include "time.h"
namespace sylar {
// 时间戳按照指定格式转换为字符串
std::string FormatTime(time_t t, const std::string& fmt) {
    struct tm tm;
    localtime_r(&t, &tm);
    char buf[64];
    size_t len = strftime(buf, sizeof(buf), fmt.c_str(), &tm);
    return std::string(buf, len);
}
// 字符串转化为时间戳
time_t ParseTime(const std::string& str, const std::string& fmt) {
    struct tm tm;
    strptime(str.c_str(), fmt.c_str(), &tm);
    return mktime(&tm);
}
}  // namespace sylar
