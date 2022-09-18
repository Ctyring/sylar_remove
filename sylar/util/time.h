#pragma once
#include <cstdint>
#include <ctime>
#include <string>
namespace sylar {
// 时间戳按照指定格式转换为字符串
std::string FormatTime(time_t t, const std::string& fmt = "%Y-%m-%d %H:%M:%S");
// 字符串转化为时间戳
time_t ParseTime(const std::string& str,
                 const std::string& fmt = "%Y-%m-%d %H:%M:%S");
}  // namespace sylar
