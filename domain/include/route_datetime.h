#ifndef ROUTE_DATETIME_H
#define ROUTE_DATETIME_H

#include <string>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>

struct RouteDatetime
{
    std::string datetime_str; // 日期时间字符串
    long timestamp;   // 时间戳
    int week_day;     // 星期

    RouteDatetime(std::string &datetime_str);
};

struct RouteDate
{
    std::string date_str; // 日期时间字符串
    long timestamp;   // 时间戳
    int week_day;     // 星期

    RouteDate(std::string &date_str);
};


#endif // ROUTE_DATETIME_H
