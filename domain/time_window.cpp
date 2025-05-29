#include "time_window.h"

TimeWindowDateTime::TimeWindowDateTime(long early, long late)
{
    std::time_t early_time = static_cast<std::time_t>(early);
    std::time_t late_time = static_cast<std::time_t>(late);
    this->early = *std::localtime(&early_time);
    this->late = *std::localtime(&late_time);
}

TimeWindow *TimeWindowDateTime::convert()
{
    return new TimeWindow(std::mktime(&this->early), std::mktime(&this->late));
}

TimeWindow::TimeWindow(int time_window_type)
{
    if (time_window_type == 0)
    {
        this->early = TIME_WINDOW_MIN;
        this->late = TIME_WINDOW_MAX;
    }
    else
    {
        this->early = INT_MIN + 1E17;
        this->late = INT_MAX - 1E17;
    }
}

TimeWindow *TimeWindow::intersection(TimeWindow *time_window)
{
    long tmp_early = std::max(this->early, time_window->early);
    long tmp_late = std::min(this->late, time_window->late);
    if (tmp_early > tmp_late)
    {
        return nullptr;
    }
    return new TimeWindow(tmp_early, tmp_late);
}

std::unique_ptr<TimeWindow> TimeWindow::intersection(std::unique_ptr<TimeWindow> &time_window)
{
    long tmp_early = std::max(this->early, time_window->early);
    long tmp_late = std::min(this->late, time_window->late);
    if (tmp_early > tmp_late)
    {
        return nullptr;
    }
    return std::make_unique<TimeWindow>(tmp_early, tmp_late);
}
