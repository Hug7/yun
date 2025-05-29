#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <vector>
#include <ctime>
#include <memory>
#include <climits>

#include "location.h"
#include "time_window.h"
#include "default_parameter.h"

struct OrderHash;

struct OrderHash
{
    // 卸货门店
    std::string delivery_id;
    // 访问优先级: 按照优先级从低到高访问，default=INT_MAX，有效取值范围[0, INT_MAX - 1], 如果不在取值范围内，则优先级最低
    int priority = DEFAULT_PRIORITY;
    // 分组标识: 相同分组标识的站点如果在同一条线路时访问顺序要相邻，default=INT_MAX，有效取值范围[0, INT_MAX - 1], 如果不在取值范围内，则无效
    std::string group_mark = DEFAULT_GROUP_MARK;
    // 分组优先级: 在分组内按照优先级从低到高访问，default=INT_MAX，有效取值范围[0, INT_MAX - 1], 如果不在取值范围内，则优先级最低
    int group_priority = DEFAULT_GROUP_PRIORITY;
    // 第一个访问
    bool first_visit = false;
    // 最后一个访问
    bool last_visit = false;

    OrderHash(std::string delivery_id, int priority, std::string group_mark, int group_priority, bool first_visit, bool last_visit)
        : delivery_id(delivery_id), priority(priority), group_mark(group_mark), group_priority(group_priority), first_visit(first_visit), last_visit(last_visit) {}

    bool operator==(OrderHash rhs) const
    {
        if (delivery_id != rhs.delivery_id)
        {
            return false;
        }
        if (priority != rhs.priority)
        {
            return false;
        }
        else if (group_mark != rhs.group_mark)
        {
            return false;
        }
        else if (group_priority != rhs.group_priority)
        {
            return false;
        }
        else if (first_visit != rhs.first_visit)
        {
            return false;
        }
        else if (last_visit != rhs.last_visit)
        {
            return false;
        }
        return true;
    }
};

namespace std
{
    template <>
    struct hash<OrderHash>
    {
        std::size_t operator()(OrderHash code) const
        {
            std::size_t h1 = std::hash<std::string>()(code.delivery_id);
            std::size_t h2 = std::hash<int>()(code.priority);
            std::size_t h3 = std::hash<std::string>()(code.group_mark);
            std::size_t h4 = std::hash<int>()(code.group_priority);
            std::size_t h5 = std::hash<bool>()(code.first_visit);
            std::size_t h6 = std::hash<bool>()(code.last_visit);
            std::size_t h = h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5);
            return h;
        }
    };
}

class Order
{
public:
    std::string id;
    std::string pickup_loc_id;
    std::string delivery_loc_id;
    int pickup_work_time;
    int delivery_work_time;

    std::unique_ptr<TimeWindowDateTime> pickup_time_window;
    std::unique_ptr<TimeWindowDateTime> delivery_time_window;

    // 非必要参数，特征有默认值，不出现在构造函数中

    // 访问优先级: 按照优先级从低到高访问，default=INT_MAX，有效取值范围[0, INT_MAX - 1], 如果不在取值范围内，则优先级最低
    int priority = DEFAULT_PRIORITY;
    // 分组标识: 相同分组标识的站点如果在同一条线路时访问顺序要相邻，default=INT_MAX + 1，有效取值范围[0, INT_MAX], 如果不在取值范围内，则无效
    std::string group_mark = DEFAULT_GROUP_MARK;
    // 分组优先级: 在分组内按照优先级从低到高访问，default=INT_MAX + 1，有效取值范围[0, INT_MAX], 如果不在取值范围内，则优先级最低
    int group_priority = DEFAULT_GROUP_PRIORITY;
    // 第一个访问
    bool first_visit = false;
    // 最后一个访问
    bool last_visit = false;

    Order(std::string id, std::string pickup_loc_id, std::string delivery_loc_id,
          int pickup_work_time, int delivery_work_time,
          std::unique_ptr<TimeWindowDateTime> pickup_time_window, std::unique_ptr<TimeWindowDateTime> delivery_time_window);

    OrderHash get_order_hash();

    ~Order() {}
};

#endif // ORDER_H
