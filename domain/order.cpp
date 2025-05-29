#include "order.h"

Order::Order(std::string id, std::string pickup_loc_id, std::string delivery_loc_id,
             int pickup_work_time, int delivery_work_time,
             std::unique_ptr<TimeWindowDateTime> pickup_time_window, std::unique_ptr<TimeWindowDateTime> delivery_time_window)
{
    this->id = id;
    this->pickup_loc_id = pickup_loc_id;
    this->delivery_loc_id = delivery_loc_id;
    this->pickup_work_time = pickup_work_time;
    this->delivery_work_time = delivery_work_time;
    this->pickup_time_window = std::move(pickup_time_window);
    this->delivery_time_window = std::move(delivery_time_window);
}

OrderHash Order::get_order_hash()
{
    return std::move(OrderHash(
        this->delivery_loc_id,
        this->priority,
        this->group_mark,
        this->group_priority,
        this->first_visit,
        this->last_visit));
}
