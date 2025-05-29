#ifndef LOCATION_H
#define LOCATION_H

#include <string>
#include <vector>
#include <memory>
#include <climits>

#include "time_window.h"
#include "calendar.h"

class Location
{
public:
    std::string id; // 唯一名称
    std::unique_ptr<Calendar> pick_calendar; // 提货日历
    std::unique_ptr<Calendar> delivery_calendar; // 卸货日历
    int work_time;    // 固定耗时
    bool first_visit; // 第一个访问
    bool last_visit;  // 最后一个访问

    Location(std::string id);

    ~Location();
};



#endif // LOCATION_H
