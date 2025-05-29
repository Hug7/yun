#include "route_datetime.h"

RouteDatetime::RouteDatetime(std::string &datetime_str)
{
    this->datetime_str = datetime_str;

    // std::tm tm = {};
    // strptime(datetime_str.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
    
    std::tm tm = {0};
    std::istringstream ss(datetime_str);
    std::string format = "%Y-%m-%d %H:%M:%S";
    ss >> std::get_time(&tm, format.c_str());

    this->timestamp = std::mktime(&tm);
    this->week_day = tm.tm_wday;
}

RouteDate::RouteDate(std::string &date_str)
{
    this->date_str = date_str;
    // std::tm tm = {};
    // strptime(date_str.c_str(), "%Y-%m-%d", &tm);

    std::tm tm = {0};
    std::istringstream ss(date_str);
    std::string format = "%Y-%m-%d";
    ss >> std::get_time(&tm, format.c_str());

    this->timestamp = std::mktime(&tm);
    this->week_day = tm.tm_wday;
}
