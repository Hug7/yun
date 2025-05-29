#ifndef TIME_WINDOW_H
#define TIME_WINDOW_H

#include <ctime>
#include <memory>

#define TIME_WINDOW_MIN -28800      // 1970-01-01 00:00:00
#define TIME_WINDOW_MAX 32503651200 // 3000-01-01 00:00:00

class TimeWindowDateTime;
class TimeWindow;


class TimeWindowDateTime
{
public:
    std::tm early;  // early time
    std::tm late;   // late time

    TimeWindowDateTime(std::tm early, std::tm late) : early(early), late(late) {}

    TimeWindowDateTime(long early, long late);

    TimeWindow *convert();

    ~TimeWindowDateTime() {}
};

class TimeWindow
{
public:
    long early; // early time
    long late;  // late time
    /**
     * time_window_type
     * 0: 1970-01-01 00:00:00 ~ 3000-01-01 00:00:00
     * !=0: INT_MIN * 100 ~ INT_MAX * 100
     */
    
    TimeWindow(int time_window_type);

    TimeWindow(long early, long late) : early(early), late(late) {}

    ~TimeWindow() {}

    // Whether the time windows intersect
    TimeWindow *intersection(TimeWindow *time_window);
    std::unique_ptr<TimeWindow> intersection(std::unique_ptr<TimeWindow> &time_window);
};

#endif // TIME_WINDOW_H
