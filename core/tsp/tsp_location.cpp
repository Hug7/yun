#include "tsp_location.h"

TspLocation::~TspLocation()
{
    for (auto &tw : this->time_windows)
    {
        delete tw;
    }
    this->time_windows.clear();
}

TspLocation *TspLocation::create_tmp_location(std::string &id, int matrix_ind, int loc_ind)
{
    TspLocation *tsp_location = new TspLocation(id, matrix_ind, loc_ind);
    tsp_location->time_windows = std::vector<TimeWindow *>();
    tsp_location->time_windows.push_back(new TimeWindow(0));
    
    tsp_location->is_tmp_loc = true;
    return tsp_location;
}

void TspLocation::reset_group_info()
{
    this->group_mark = DEFAULT_GROUP_MARK;
    this->group_priority = DEFAULT_GROUP_PRIORITY;
    this->is_default_group_mark = true;
}

void TspLocation::update_relation_vars()
{
    this->is_default_group_mark = this->group_mark == DEFAULT_GROUP_MARK;
}
