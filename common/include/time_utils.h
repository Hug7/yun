#ifndef TimeUtils_H
#define TimeUtils_H

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>


std::string timestampToFormattedTime(std::time_t timestamp)
{
    // 将时间戳转换为本地时间
    std::tm *localTime = std::localtime(&timestamp);

    // 使用 stringstream 和 put_time 格式化时间
    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

std::tm get_date_tm(std::tm &t)
{
    std::tm date_tm = {};
    date_tm.tm_year = t.tm_year;
    date_tm.tm_mon = t.tm_mon;
    date_tm.tm_mday = t.tm_mday;
    
    return date_tm;
}

std::tm plus_day(std::tm &t, int day)
{
    std::time_t timestamp = std::mktime(&t) + 86400 * day;
    return *std::localtime(&timestamp);
}

#endif // TimeUtils_H