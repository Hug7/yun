/*
 * chrono_util.hpp — Self-contained C++ date/time/timezone library
 *
 * Design goals:
 *   - No dependency on <chrono>, <ctime> timezone functions, or OS tzdata.
 *   - All date math uses pure integer arithmetic; deterministic on every platform.
 *   - Embedded timezone database (IANA names + UTC offsets + DST rules).
 *   - Header-only; drop it in and #include.
 *
 * Author: Codex
 * License: MIT
 */

#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>
#include <ctime>
#include <stdexcept>

namespace chrono_util
{

    // ============================================================================
    // 1. Constants
    // ============================================================================

    constexpr int64_t SEC_PER_MIN = 60;
    constexpr int64_t SEC_PER_HOUR = 3600;
    constexpr int64_t SEC_PER_DAY = 86400;

    // Unix epoch: 1970-01-01 00:00:00 UTC
    constexpr int64_t UNIX_EPOCH_SERIAL = 719468; // serial day number of 1970-01-01

    // Internal global default timezone  (defined here so forward-references resolve)
    inline char _g_default_tz_name[64] = "Asia/Shanghai";

    // Sentinel: distinguishes "explicit UTC offset parsed" from "no timezone at all"
    inline const char TZ_EXPLICIT_OFFSET[] = "";

    // ============================================================================
    // 2. Core types
    // ============================================================================

    struct Date
    {
        int year;  // full year, e.g. 2024 (negative = BCE)
        int month; // 1..12
        int day;   // 1..31
    };

    struct Time
    {
        int hour;       // 0..23
        int minute;     // 0..59
        int second;     // 0..59 (leap seconds clamped to 59)
        int nanosecond; // 0..999999999
    };

    struct DateTime
    {
        Date date;
        Time time;
        int utc_offset_minutes; // offset from UTC in minutes, e.g. +480 for Asia/Shanghai
        const char *tz_name;    // IANA name or "+HH:MM", or nullptr

        DateTime() : date{1970, 1, 1}, time{0, 0, 0, 0}, utc_offset_minutes(0), tz_name(nullptr) {}
    };

    // ============================================================================
    // 3. Validation helpers
    // ============================================================================

    inline bool is_leap_year(int y)
    {
        return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    }

    inline int days_in_month(int y, int m)
    {
        static const int d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (m < 1 || m > 12)
            return 0;
        if (m == 2 && is_leap_year(y))
            return 29;
        return d[m - 1];
    }

    inline bool valid_date(const Date &d)
    {
        if (d.month < 1 || d.month > 12)
            return false;
        if (d.day < 1 || d.day > days_in_month(d.year, d.month))
            return false;
        return true;
    }

    inline bool valid_time(const Time &t)
    {
        return t.hour >= 0 && t.hour < 24 && t.minute >= 0 && t.minute < 60 && t.second >= 0 && t.second < 60 && t.nanosecond >= 0 && t.nanosecond < 1000000000;
    }

    // Day of week (0 = Sunday, 1 = Monday, ... 6 = Saturday)
    // Uses Zeller-like algorithm on serial day number.
    inline int day_of_week(int64_t serial)
    {
        // 1970-01-01 was a Thursday (4). serial=0 corresponds to that day.
        return (int)((serial + 4) % 7 + 7) % 7;
    }

    inline int day_of_week(const Date &d)
    {
        // Use the serial-number path below — defined later, declared here for convenience.
        // We inline a minimal serial computation to avoid forward-declaration issues.
        // days_from_civil inline version:
        auto cd = [](int y, int m, int d_) -> int64_t
        {
            y -= (m <= 2) ? 1 : 0;
            int64_t era = (y >= 0 ? y : y - 399) / 400;
            int64_t yoe = y - era * 400;
            int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d_ - 1;
            int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
            return era * 146097 + doe - 719468;
        };
        return day_of_week(cd(d.year, d.month, d.day));
    }

    inline int day_of_week(const DateTime &dt)
    {
        return day_of_week(dt.date);
    }

    // ============================================================================
    // 4. Serial day number — civil <-> serial (Howard Hinnant algorithm)
    // ============================================================================

    // Convert year/month/day to days since 1970-01-01.
    inline int64_t days_from_civil(int y, int m, int d_)
    {
        y -= (m <= 2) ? 1 : 0;
        int64_t era = (y >= 0 ? y : y - 399) / 400;
        int64_t yoe = y - era * 400;
        int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d_ - 1;
        int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097 + doe - 719468;
    }

    // Convert days since 1970-01-01 to year/month/day.
    inline Date civil_from_days(int64_t z)
    {
        z += 719468;
        int64_t era = (z >= 0 ? z : z - 146096) / 146097;
        int64_t doe = z - era * 146097;
        int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        int64_t y = yoe + era * 400;
        int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        int64_t mp = (5 * doy + 2) / 153;
        int64_t d_ = doy - (153 * mp + 2) / 5 + 1;
        int64_t m = mp + (mp < 10 ? 3 : -9);
        y += (m <= 2) ? 1 : 0;
        return {(int)y, (int)m, (int)d_};
    }

    inline int64_t days_from_civil(const Date &d)
    {
        return days_from_civil(d.year, d.month, d.day);
    }

    // ============================================================================
    // 5. Timestamp conversion (seconds since Unix epoch, UTC)
    // ============================================================================

    // Convert a DateTime to a Unix timestamp.  The DateTime's utc_offset_minutes
    // is used to compute the absolute UTC instant.
    inline int64_t to_timestamp(const DateTime &dt)
    {
        int64_t days = days_from_civil(dt.date);
        int64_t sec_of_day = (int64_t)dt.time.hour * 3600 + (int64_t)dt.time.minute * 60 + (int64_t)dt.time.second;
        return days * 86400 + sec_of_day - (int64_t)dt.utc_offset_minutes * 60;
    }

    // Convert a Unix timestamp to a DateTime interpreted in a given UTC offset.
    inline DateTime from_timestamp(int64_t ts, int utc_offset_minutes, const char *tz_name = nullptr)
    {
        // Apply offset so the local wall-clock time is correct.
        int64_t local_ts = ts + (int64_t)utc_offset_minutes * 60;
        int64_t days = (local_ts >= 0 ? local_ts / 86400 : (local_ts - 86399) / 86400);
        int64_t sec_of_day = local_ts - days * 86400;
        if (sec_of_day < 0)
        {
            sec_of_day += 86400;
            days -= 1;
        }

        DateTime dt;
        dt.date = civil_from_days(days);
        dt.time.hour = (int)(sec_of_day / 3600);
        dt.time.minute = (int)((sec_of_day % 3600) / 60);
        dt.time.second = (int)(sec_of_day % 60);
        dt.time.nanosecond = 0;
        dt.utc_offset_minutes = utc_offset_minutes;
        dt.tz_name = tz_name;
        return dt;
    }


    // Comparison operators — compare UTC instants via to_timestamp()
    inline bool operator==(const DateTime &a, const DateTime &b)
    {
        return to_timestamp(a) == to_timestamp(b);
    }
    inline bool operator!=(const DateTime &a, const DateTime &b)
    {
        return to_timestamp(a) != to_timestamp(b);
    }
    inline bool operator<(const DateTime &a, const DateTime &b)
    {
        return to_timestamp(a) < to_timestamp(b);
    }
    inline bool operator<=(const DateTime &a, const DateTime &b)
    {
        return to_timestamp(a) <= to_timestamp(b);
    }
    inline bool operator>(const DateTime &a, const DateTime &b)
    {
        return to_timestamp(a) > to_timestamp(b);
    }
    inline bool operator>=(const DateTime &a, const DateTime &b)
    {
        return to_timestamp(a) >= to_timestamp(b);
    }

    // ============================================================================
    // 6. Date/time arithmetic
    // ============================================================================

    inline DateTime add_days(const DateTime &dt, int64_t ndays)
    {
        int64_t ts = to_timestamp(dt);
        return from_timestamp(ts + ndays * 86400, dt.utc_offset_minutes, dt.tz_name);
    }

    inline DateTime add_hours(const DateTime &dt, int64_t nhours)
    {
        int64_t ts = to_timestamp(dt);
        return from_timestamp(ts + nhours * 3600, dt.utc_offset_minutes, dt.tz_name);
    }

    inline DateTime add_minutes(const DateTime &dt, int64_t nmins)
    {
        int64_t ts = to_timestamp(dt);
        return from_timestamp(ts + nmins * 60, dt.utc_offset_minutes, dt.tz_name);
    }

    inline DateTime add_seconds(const DateTime &dt, int64_t nsecs)
    {
        int64_t ts = to_timestamp(dt);
        return from_timestamp(ts + nsecs, dt.utc_offset_minutes, dt.tz_name);
    }

    // Difference in days between two DateTimes (both converted to UTC first).
    inline int64_t diff_days(const DateTime &a, const DateTime &b)
    {
        int64_t ta = to_timestamp(a);
        int64_t tb = to_timestamp(b);
        return (ta - tb) / 86400;
    }

    // ============================================================================
    // 7. Parsing
    // ============================================================================

    // --- Internal helpers ---

    inline int parse_int2(const char *&p)
    {
        int v = (p[0] - '0') * 10 + (p[1] - '0');
        p += 2;
        return v;
    }

    inline int parse_int4(const char *&p)
    {
        int v = (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 + (p[3] - '0');
        p += 4;
        return v;
    }

    inline void skip_char(const char *&p, char c)
    {
        if (*p == c)
            ++p;
    }

    inline void skip_ws(const char *&p)
    {
        while (*p == ' ' || *p == '\t')
            ++p;
    }

    // strptime-style format parser.
    // Supported specifiers:
    //   %Y — 4-digit year        %m — 2-digit month (01-12)
    //   %d — 2-digit day          %H — 2-digit hour (00-23)
    //   %M — 2-digit minute       %S — 2-digit second
    //   %z — +HHMM or -HHMM       %Z — timezone name (literal, read until whitespace)
    //   %b — abbreviated month    %B — full month name
    //   %I — 12-hour hour         %p — AM/PM
    //   %F — %Y-%m-%d (ISO date)
    //   %% — literal %

    // Forward declaration — definition is in section 12 (timezone conversion).
    inline void _apply_default_timezone(DateTime &dt);
    // Separators in the format string are matched literally.
    inline bool parse_format(const char *s, const char *fmt, DateTime &out)
    {
        const char *p = s;
        const char *f = fmt;
        int Y = 1970, M = 1, D = 1, h = 0, mi = 0, sec = 0;
        int tz_off = 0;
        const char *tz_n = nullptr;
        bool is_pm = false;
        bool have_12h = false;

        static const char *months_abbr[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        static const char *months_full[] = {
            "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"};

        while (*f)
        {
            if (*f == '%')
            {
                ++f;
                switch (*f)
                {
                case 'Y':
                    if (p[0] < '0' || p[0] > '9')
                        return false;
                    Y = parse_int4(p);
                    break;
                case 'm':
                    if (p[0] < '0' || p[0] > '9')
                        return false;
                    M = parse_int2(p);
                    break;
                case 'd':
                    if (p[0] < '0' || p[0] > '9')
                        return false;
                    D = parse_int2(p);
                    break;
                case 'H':
                    if (p[0] < '0' || p[0] > '9')
                        return false;
                    h = parse_int2(p);
                    break;
                case 'I':
                    if (p[0] < '0' || p[0] > '9')
                        return false;
                    h = parse_int2(p);
                    have_12h = true;
                    break;
                case 'M':
                    if (p[0] < '0' || p[0] > '9')
                        return false;
                    mi = parse_int2(p);
                    break;
                case 'S':
                    if (p[0] < '0' || p[0] > '9')
                        return false;
                    sec = parse_int2(p);
                    break;
                case 'p':
                    if (strncasecmp(p, "AM", 2) == 0)
                    {
                        is_pm = false;
                        p += 2;
                    }
                    else if (strncasecmp(p, "PM", 2) == 0)
                    {
                        is_pm = true;
                        p += 2;
                    }
                    else
                        return false;
                    break;
                case 'z':
                {
                    if (*p == 'Z' || *p == 'z')
                    {
                        ++p;
                        tz_off = 0;
                        tz_n = "UTC";
                    }
                    else if (*p == '+' || *p == '-')
                    {
                        int sign = (*p == '+') ? 1 : -1;
                        ++p;
                        tz_off = sign * (parse_int2(p) * 60);
                        if (*p == ':')
                            ++p; // skip colon in +HH:MM
                        if (*p >= '0' && *p <= '9')
                            tz_off += sign * parse_int2(p);
                        tz_n = TZ_EXPLICIT_OFFSET;
                    }
                    break;
                }
                case 'Z':
                {
                    // Read timezone name until whitespace or end
                    const char *start = p;
                    while (*p && *p != ' ' && *p != '\t')
                        ++p;
                    // We'll look up name later; store temporarily
                    tz_n = start; // caller must ensure null-termination or copy
                    break;
                }
                case 'b':
                {
                    bool found = false;
                    for (int i = 0; i < 12; ++i)
                    {
                        size_t len = strlen(months_abbr[i]);
                        if (strncasecmp(p, months_abbr[i], len) == 0 && (p[len] == ' ' || p[len] == '-' || p[len] == ',' || p[len] == 0))
                        {
                            M = i + 1;
                            p += len;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        return false;
                    break;
                }
                case 'B':
                {
                    bool found = false;
                    for (int i = 0; i < 12; ++i)
                    {
                        size_t len = strlen(months_full[i]);
                        if (strncasecmp(p, months_full[i], len) == 0)
                        {
                            M = i + 1;
                            p += len;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        return false;
                    break;
                }
                case 'F':
                {
                    const char *pp = p;
                    Y = parse_int4(pp);
                    skip_char(pp, '-');
                    M = parse_int2(pp);
                    skip_char(pp, '-');
                    D = parse_int2(pp);
                    p = pp;
                    break;
                }
                case '%':
                    skip_char(p, '%');
                    break;
                default:
                    // Unknown specifier — skip
                    break;
                }
                ++f;
            }
            else if (*f == ' ')
            {
                skip_ws(p);
                ++f;
            }
            else
            {
                if (*p != *f)
                    return false;
                ++p;
                ++f;
            }
        }

        if (have_12h)
        {
            if (h == 12)
                h = 0;
            if (is_pm)
                h += 12;
        }

        out.date = {Y, M, D};
        out.time = {h, mi, sec, 0};
        out.utc_offset_minutes = tz_off;
        out.tz_name = tz_n;

        if (!valid_date(out.date) || !valid_time(out.time))
            return false;
        _apply_default_timezone(out);
        return true;
    }

    // Parse with an explicit format string.  Delegates to parse_format.
    inline bool parse(const char *s, const char *fmt, DateTime &out)
    {
        return parse_format(s, fmt, out);
    }

    // Convenience overload: string + format, returns DateTime by value.
    inline DateTime parse(const std::string &s, const char *fmt)
    {
        DateTime dt;
        if (!parse(s.c_str(), fmt, dt))
        {
            throw std::runtime_error("Invalid date/time string " + s + ", fmt:" + std::string(fmt));
        }
        return dt;
    }

    // ============================================================================
    // 8. Formatting
    // ============================================================================

    // Format a DateTime into a string using strftime-like specifiers.
    // Specifiers:
    //   %Y %m %d %H %M %S — as expected
    //   %b — abbreviated month name
    //   %B — full month name
    //   %a — abbreviated weekday
    //   %A — full weekday
    //   %w — weekday number (0=Sunday)
    //   %z — timezone offset as +HHMM
    //   %Z — timezone name (or offset string)
    //   %F — %Y-%m-%d
    //   %T — %H:%M:%S
    //   %p — AM/PM
    //   %I — 12-hour hour
    //   %% — literal %
    inline std::string format(const DateTime &dt, const char *fmt)
    {
        static const char *wday_abbr[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        static const char *wday_full[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
        static const char *mon_abbr[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        static const char *mon_full[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

        int wd = day_of_week(dt.date);
        std::string result;
        result.reserve(strlen(fmt) + 32);

        for (const char *f = fmt; *f; ++f)
        {
            if (*f == '%')
            {
                ++f;
                char buf[64];
                switch (*f)
                {
                case 'Y':
                    snprintf(buf, sizeof(buf), "%04d", dt.date.year);
                    break;
                case 'm':
                    snprintf(buf, sizeof(buf), "%02d", dt.date.month);
                    break;
                case 'd':
                    snprintf(buf, sizeof(buf), "%02d", dt.date.day);
                    break;
                case 'H':
                    snprintf(buf, sizeof(buf), "%02d", dt.time.hour);
                    break;
                case 'M':
                    snprintf(buf, sizeof(buf), "%02d", dt.time.minute);
                    break;
                case 'S':
                    snprintf(buf, sizeof(buf), "%02d", dt.time.second);
                    break;
                case 'I':
                {
                    int h12 = dt.time.hour % 12;
                    if (h12 == 0)
                        h12 = 12;
                    snprintf(buf, sizeof(buf), "%02d", h12);
                    break;
                }
                case 'p':
                    snprintf(buf, sizeof(buf), "%s", dt.time.hour < 12 ? "AM" : "PM");
                    break;
                case 'a':
                    snprintf(buf, sizeof(buf), "%s", wday_abbr[wd]);
                    break;
                case 'A':
                    snprintf(buf, sizeof(buf), "%s", wday_full[wd]);
                    break;
                case 'b':
                    snprintf(buf, sizeof(buf), "%s", mon_abbr[dt.date.month - 1]);
                    break;
                case 'B':
                    snprintf(buf, sizeof(buf), "%s", mon_full[dt.date.month - 1]);
                    break;
                case 'w':
                    snprintf(buf, sizeof(buf), "%d", wd);
                    break;
                case 'z':
                {
                    int off = dt.utc_offset_minutes;
                    int sign = off < 0 ? -1 : 1;
                    int ah = (off * sign) / 60;
                    int am = (off * sign) % 60;
                    snprintf(buf, sizeof(buf), "%c%02d%02d", off < 0 ? '-' : '+', ah, am);
                    break;
                }
                case 'Z':
                    if (dt.tz_name && dt.tz_name[0])
                    {
                        snprintf(buf, sizeof(buf), "%s", dt.tz_name);
                    }
                    else
                    {
                        format(dt, "%z").copy(buf, 64);
                    }
                    break;
                case 'F':
                    result += format(dt, "%Y-%m-%d");
                    buf[0] = 0;
                    break;
                case 'T':
                    result += format(dt, "%H:%M:%S");
                    buf[0] = 0;
                    break;
                case '%':
                    buf[0] = '%';
                    buf[1] = 0;
                    break;
                default:
                    buf[0] = *f;
                    buf[1] = 0;
                    break;
                }
                result += buf;
            }
            else
            {
                result += *f;
            }
        }
        return result;
    }

    inline std::string format_iso8601(const DateTime &dt)
    {
        std::string s = format(dt, "%Y-%m-%dT%H:%M:%S");
        if (dt.tz_name && strcmp(dt.tz_name, "UTC") == 0 && dt.utc_offset_minutes == 0)
        {
            s += "Z";
        }
        else
        {
            s += format(dt, "%z");
        }
        return s;
    }

    // ============================================================================
    // 9. Timezone database
    // ============================================================================

    struct TzEntry
    {
        const char *name; // IANA timezone name, e.g. "Asia/Shanghai"
        int utc_offset;   // standard-offset in minutes from UTC, e.g. +480
        bool has_dst;     // true if this zone observes daylight-saving time
        int dst_offset;   // additional offset during DST in minutes (typically +60)
    };

    // Sorted alphabetically by name for binary search.
    // Covers ~130 commonly-used IANA timezones.
    // DST rules are computed algorithmically for zones with has_dst=true;
    // zones with has_dst=false or for dates outside the DST window simply use utc_offset.
    const TzEntry TZ_DB[] = {
        {"Africa/Abidjan", 0, false, 0},
        {"Africa/Accra", 0, false, 0},
        {"Africa/Addis_Ababa", 180, false, 0},
        {"Africa/Algiers", 60, false, 0},
        {"Africa/Cairo", 120, false, 0},
        {"Africa/Casablanca", 0, true, 60},
        {"Africa/Johannesburg", 120, false, 0},
        {"Africa/Lagos", 60, false, 0},
        {"Africa/Maputo", 120, false, 0},
        {"Africa/Nairobi", 180, false, 0},
        {"Africa/Tripoli", 120, false, 0},
        {"Africa/Tunis", 60, false, 0},
        {"America/Anchorage", -540, true, 60},
        {"America/Argentina/Buenos_Aires", -180, false, 0},
        {"America/Bogota", -300, false, 0},
        {"America/Boise", -420, true, 60},
        {"America/Caracas", -240, false, 0},
        {"America/Chicago", -360, true, 60},
        {"America/Denver", -420, true, 60},
        {"America/Detroit", -300, true, 60},
        {"America/Godthab", -180, true, 60},
        {"America/Guatemala", -360, false, 0},
        {"America/Halifax", -240, true, 60},
        {"America/Indiana/Indianapolis", -300, true, 60},
        {"America/La_Paz", -240, false, 0},
        {"America/Lima", -300, false, 0},
        {"America/Los_Angeles", -480, true, 60},
        {"America/Manaus", -240, false, 0},
        {"America/Mexico_City", -360, false, 0},
        {"America/New_York", -300, true, 60},
        {"America/Noronha", -120, false, 0},
        {"America/Phoenix", -420, false, 0},
        {"America/Santiago", -240, true, 60},
        {"America/Sao_Paulo", -180, false, 0},
        {"America/St_Johns", -150, true, 60},
        {"America/Tijuana", -480, true, 60},
        {"America/Toronto", -300, true, 60},
        {"America/Vancouver", -480, true, 60},
        {"America/Winnipeg", -360, true, 60},
        {"Asia/Almaty", 300, false, 0},
        {"Asia/Amman", 120, true, 60},
        {"Asia/Baghdad", 180, false, 0},
        {"Asia/Baku", 240, false, 0},
        {"Asia/Bangkok", 420, false, 0},
        {"Asia/Beirut", 120, true, 60},
        {"Asia/Calcutta", 330, false, 0},
        {"Asia/Chongqing", 480, false, 0},
        {"Asia/Colombo", 330, false, 0},
        {"Asia/Damascus", 120, true, 60},
        {"Asia/Dhaka", 360, false, 0},
        {"Asia/Dubai", 240, false, 0},
        {"Asia/Hong_Kong", 480, false, 0},
        {"Asia/Irkutsk", 480, false, 0},
        {"Asia/Jakarta", 420, false, 0},
        {"Asia/Jerusalem", 120, true, 60},
        {"Asia/Kabul", 270, false, 0},
        {"Asia/Kamchatka", 720, false, 0},
        {"Asia/Karachi", 300, false, 0},
        {"Asia/Kathmandu", 345, false, 0},
        {"Asia/Kolkata", 330, false, 0},
        {"Asia/Krasnoyarsk", 420, false, 0},
        {"Asia/Kuala_Lumpur", 480, false, 0},
        {"Asia/Kuwait", 180, false, 0},
        {"Asia/Magadan", 660, false, 0},
        {"Asia/Manila", 480, false, 0},
        {"Asia/Nicosia", 120, true, 60},
        {"Asia/Novosibirsk", 420, false, 0},
        {"Asia/Omsk", 360, false, 0},
        {"Asia/Qatar", 180, false, 0},
        {"Asia/Rangoon", 390, false, 0},
        {"Asia/Riyadh", 180, false, 0},
        {"Asia/Seoul", 540, false, 0},
        {"Asia/Shanghai", 480, false, 0},
        {"Asia/Singapore", 480, false, 0},
        {"Asia/Taipei", 480, false, 0},
        {"Asia/Tashkent", 300, false, 0},
        {"Asia/Tbilisi", 240, false, 0},
        {"Asia/Tehran", 210, true, 60},
        {"Asia/Tokyo", 540, false, 0},
        {"Asia/Ulaanbaatar", 480, false, 0},
        {"Asia/Urumqi", 360, false, 0},
        {"Asia/Vladivostok", 600, false, 0},
        {"Asia/Yakutsk", 540, false, 0},
        {"Asia/Yekaterinburg", 300, false, 0},
        {"Asia/Yerevan", 240, false, 0},
        {"Atlantic/Azores", -60, true, 60},
        {"Atlantic/Cape_Verde", -60, false, 0},
        {"Atlantic/South_Georgia", -120, false, 0},
        {"Atlantic/Stanley", -180, false, 0},
        {"Australia/Adelaide", 570, true, 60},
        {"Australia/Brisbane", 600, false, 0},
        {"Australia/Darwin", 570, false, 0},
        {"Australia/Hobart", 600, true, 60},
        {"Australia/Perth", 480, false, 0},
        {"Australia/Sydney", 600, true, 60},
        {"Etc/GMT", 0, false, 0},
        {"Etc/GMT+1", -60, false, 0},
        {"Etc/GMT+10", -600, false, 0},
        {"Etc/GMT+11", -660, false, 0},
        {"Etc/GMT+12", -720, false, 0},
        {"Etc/GMT+2", -120, false, 0},
        {"Etc/GMT+3", -180, false, 0},
        {"Etc/GMT+4", -240, false, 0},
        {"Etc/GMT+5", -300, false, 0},
        {"Etc/GMT+6", -360, false, 0},
        {"Etc/GMT+7", -420, false, 0},
        {"Etc/GMT+8", -480, false, 0},
        {"Etc/GMT+9", -540, false, 0},
        {"Etc/GMT-1", 60, false, 0},
        {"Etc/GMT-10", 600, false, 0},
        {"Etc/GMT-11", 660, false, 0},
        {"Etc/GMT-12", 720, false, 0},
        {"Etc/GMT-13", 780, false, 0},
        {"Etc/GMT-14", 840, false, 0},
        {"Etc/GMT-2", 120, false, 0},
        {"Etc/GMT-3", 180, false, 0},
        {"Etc/GMT-4", 240, false, 0},
        {"Etc/GMT-5", 300, false, 0},
        {"Etc/GMT-6", 360, false, 0},
        {"Etc/GMT-7", 420, false, 0},
        {"Etc/GMT-8", 480, false, 0},
        {"Etc/GMT-9", 540, false, 0},
        {"Etc/UCT", 0, false, 0},
        {"Etc/UTC", 0, false, 0},
        {"Europe/Amsterdam", 60, true, 60},
        {"Europe/Athens", 120, true, 60},
        {"Europe/Belgrade", 60, true, 60},
        {"Europe/Berlin", 60, true, 60},
        {"Europe/Brussels", 60, true, 60},
        {"Europe/Bucharest", 120, true, 60},
        {"Europe/Budapest", 60, true, 60},
        {"Europe/Copenhagen", 60, true, 60},
        {"Europe/Dublin", 0, true, 60},
        {"Europe/Helsinki", 120, true, 60},
        {"Europe/Istanbul", 180, false, 0},
        {"Europe/Kaliningrad", 120, false, 0},
        {"Europe/Kyiv", 120, true, 60},
        {"Europe/Lisbon", 0, true, 60},
        {"Europe/London", 0, true, 60},
        {"Europe/Madrid", 60, true, 60},
        {"Europe/Minsk", 180, false, 0},
        {"Europe/Moscow", 180, false, 0},
        {"Europe/Paris", 60, true, 60},
        {"Europe/Prague", 60, true, 60},
        {"Europe/Riga", 120, true, 60},
        {"Europe/Rome", 60, true, 60},
        {"Europe/Samara", 240, false, 0},
        {"Europe/Stockholm", 60, true, 60},
        {"Europe/Tallinn", 120, true, 60},
        {"Europe/Vienna", 60, true, 60},
        {"Europe/Vilnius", 120, true, 60},
        {"Europe/Warsaw", 60, true, 60},
        {"Europe/Zurich", 60, true, 60},
        {"Indian/Chagos", 360, false, 0},
        {"Indian/Maldives", 300, false, 0},
        {"Indian/Mauritius", 240, false, 0},
        {"Pacific/Auckland", 720, true, 60},
        {"Pacific/Chatham", 765, true, 60},
        {"Pacific/Easter", -360, true, 60},
        {"Pacific/Fiji", 720, false, 0},
        {"Pacific/Guam", 600, false, 0},
        {"Pacific/Honolulu", -600, false, 0},
        {"Pacific/Kiritimati", 840, false, 0},
        {"Pacific/Norfolk", 660, true, 60},
        {"Pacific/Noumea", 660, false, 0},
        {"Pacific/Pago_Pago", -660, false, 0},
        {"Pacific/Port_Moresby", 600, false, 0},
        {"Pacific/Tahiti", -600, false, 0},
        {"Pacific/Tarawa", 720, false, 0},
        {"Pacific/Tongatapu", 780, false, 0},
        {"UTC", 0, false, 0},
    };

    constexpr int TZ_DB_SIZE = sizeof(TZ_DB) / sizeof(TZ_DB[0]);

    // Binary search the timezone database.
    inline const TzEntry *find_tz_entry(const char *name)
    {
        if (!name || !*name)
            return nullptr;
        int lo = 0, hi = TZ_DB_SIZE - 1;
        while (lo <= hi)
        {
            int mid = (lo + hi) / 2;
            int cmp = strcmp(name, TZ_DB[mid].name);
            if (cmp == 0)
                return &TZ_DB[mid];
            if (cmp < 0)
                hi = mid - 1;
            else
                lo = mid + 1;
        }
        return nullptr;
    }

    // ============================================================================
    // 10. DST rule engine
    // ============================================================================

    // DST transition rules implemented for:
    //   - North America (US/Canada): 2nd Sunday March -> 1st Sunday November (since 2007)
    //   - Europe (EU): last Sunday March -> last Sunday October
    //   - Australia: first Sunday October -> first Sunday April
    //   - Generic southern hemisphere: October -> March (approximate)
    //
    // Rules are applied based on heuristics matching the timezone's standard offset
    // and region prefix.

    // Find the Nth occurrence of a weekday in a month.
    // weekday: 0=Sunday, N: 1=first, 2=second, ... (use 5 for "last")
    inline int nth_weekday_of_month(int year, int month, int weekday, int n)
    {
        // First day of the month
        int64_t first_serial = days_from_civil(year, month, 1);
        int first_wd = day_of_week(first_serial);
        // Days until the first occurrence of `weekday`
        int days_to_first = (weekday - first_wd + 7) % 7;
        int day = 1 + days_to_first + (n - 1) * 7;
        int dim = days_in_month(year, month);
        if (day > dim)
            day -= 7; // "last" overflows to previous week
        return day;
    }

    // Determine if a given UTC timestamp falls within DST for a specific timezone entry.
    // Returns the effective UTC offset in minutes (standard + DST if applicable).
    inline int effective_utc_offset(const TzEntry *tz, int64_t utc_ts)
    {
        if (!tz || !tz->has_dst)
        {
            return tz ? tz->utc_offset : 0;
        }

        // Determine the date in the zone's local time for DST check.
        // We use the standard offset to get an approximate local date, then refine.
        int64_t local_approx = utc_ts + tz->utc_offset * 60;
        int64_t days = (local_approx >= 0 ? local_approx / 86400 : (local_approx - 86399) / 86400);
        if (local_approx < 0)
        {
            days -= (local_approx % 86400 != 0) ? 0 : 0;
        }
        // Safer:
        days = (local_approx / 86400) - (local_approx < 0 && local_approx % 86400 != 0 ? 1 : 0);
        Date local_date = civil_from_days(days);
        int year = local_date.year;
        int month = local_date.month;
        int day = local_date.day;

        // Heuristics based on region
        const char *name = tz->name;

        // --- North America (since 2007): 2nd Sun Mar -> 1st Sun Nov ---
        auto is_north_america = [&]() -> bool
        {
            return (strncmp(name, "America/", 8) == 0 && tz->utc_offset < -120) ||
                   (strcmp(name, "America/St_Johns") == 0);
        };

        // --- Europe / EU: last Sun Mar -> last Sun Oct ---
        auto is_europe = [&]() -> bool
        {
            return strncmp(name, "Europe/", 7) == 0 ||
                   strncmp(name, "Atlantic/", 9) == 0 ||
                   strcmp(name, "Asia/Nicosia") == 0 ||
                   strcmp(name, "Asia/Jerusalem") == 0;
        };

        // --- Australia: 1st Sun Oct -> 1st Sun Apr ---
        auto is_australia = [&]() -> bool
        {
            return strncmp(name, "Australia/", 10) == 0;
        };

        // --- Middle East (approximate): last Fri Mar -> last Fri Oct ---
        auto is_middle_east = [&]() -> bool
        {
            return strcmp(name, "Asia/Amman") == 0 ||
                   strcmp(name, "Asia/Beirut") == 0 ||
                   strcmp(name, "Asia/Damascus") == 0 ||
                   strcmp(name, "Africa/Casablanca") == 0;
        };

        // --- Iran: Mar 21/22 -> Sep 21/22 ---
        auto is_iran = [&]() -> bool
        {
            return strcmp(name, "Asia/Tehran") == 0;
        };

        // --- Chile: varies; approximate as Oct -> Mar (southern hemisphere) ---
        auto is_chile = [&]() -> bool
        {
            return strcmp(name, "America/Santiago") == 0 ||
                   strcmp(name, "Pacific/Easter") == 0;
        };

        // --- Pacific: Norfolk ---
        auto is_norfolk = [&]() -> bool
        {
            return strcmp(name, "Pacific/Norfolk") == 0;
        };

        // --- Chatham ---
        auto is_chatham = [&]() -> bool
        {
            return strcmp(name, "Pacific/Chatham") == 0;
        };

        int mar_day, nov_day, oct_day, apr_day;
        bool in_dst = false;

        if (is_north_america())
        {
            // US/Canada: 2nd Sun Mar 02:00 -> 1st Sun Nov 02:00
            mar_day = nth_weekday_of_month(year, 3, 0, 2);
            nov_day = nth_weekday_of_month(year, 11, 0, 1);
            if (month > 3 && month < 11)
                in_dst = true;
            else if (month == 3 && (day > mar_day || (day == mar_day && local_date.year >= 2007)))
                in_dst = true;
            else if (month == 11 && day < nov_day)
                in_dst = true;
        }
        else if (is_europe())
        {
            // EU: last Sun Mar 01:00 UTC -> last Sun Oct 01:00 UTC
            mar_day = nth_weekday_of_month(year, 3, 0, 5);
            oct_day = nth_weekday_of_month(year, 10, 0, 5);
            if (month > 3 && month < 10)
                in_dst = true;
            else if (month == 3 && (day > mar_day || (day == mar_day)))
                in_dst = true;
            else if (month == 10 && day < oct_day)
                in_dst = true;
        }
        else if (is_australia())
        {
            // Australia: 1st Sun Oct -> 1st Sun Apr (southern hemisphere)
            oct_day = nth_weekday_of_month(year, 10, 0, 1);
            apr_day = nth_weekday_of_month(year, 4, 0, 1);
            if (month > 10 || month < 4)
                in_dst = true;
            else if (month == 10 && day >= oct_day)
                in_dst = true;
            else if (month == 4 && day < apr_day)
                in_dst = true;
        }
        else if (is_middle_east())
        {
            // Approximate: last Fri Mar -> last Fri Oct
            mar_day = nth_weekday_of_month(year, 3, 5, 5);
            oct_day = nth_weekday_of_month(year, 10, 5, 5);
            if (month > 3 && month < 10)
                in_dst = true;
            else if (month == 3 && day >= mar_day)
                in_dst = true;
            else if (month == 10 && day < oct_day)
                in_dst = true;
        }
        else if (is_iran())
        {
            // Iran: ~Mar 21 -> ~Sep 21
            if (month > 3 && month < 9)
                in_dst = true;
            else if (month == 3 && day >= 21)
                in_dst = true;
            else if (month == 9 && day < 22)
                in_dst = true;
        }
        else if (is_chile())
        {
            // Chile: southern hemisphere, ~Sep -> ~Apr
            if (month > 9 || month < 4)
                in_dst = true;
            else if (month == 9 && day >= 1)
                in_dst = true;
            else if (month == 4 && day < 1)
                in_dst = true;
        }
        else if (is_norfolk() || is_chatham())
        {
            // Southern hemisphere DST: Oct -> Apr
            if (month > 9 || month < 4)
                in_dst = true;
            else if (month == 9 && day >= 28)
                in_dst = true;
            else if (month == 4 && day < 7)
                in_dst = true;
        }

        return in_dst ? tz->utc_offset + tz->dst_offset : tz->utc_offset;
    }

    // ============================================================================
    // 11. Timezone parsing (names and fixed offsets)
    // ============================================================================

    // Parse a timezone specifier. Returns the UTC offset in minutes.
    //   "UTC"     -> 0
    //   "+08:00"  -> 480
    //   "-05:00"  -> -300
    //   "+0800"   -> 480
    //   "Asia/Shanghai" -> 480
    // Returns true if the offset was determined.
    inline bool parse_timezone_offset(const char *spec, int &offset_minutes, const char *&name_out)
    {
        if (!spec || !*spec)
            return false;

        // Fixed offset: +HH:MM, -HH:MM, +HHMM, -HHMM, Z
        if (spec[0] == 'Z' || spec[0] == 'z')
        {
            offset_minutes = 0;
            name_out = "UTC";
            return true;
        }
        if (spec[0] == '+' || spec[0] == '-')
        {
            int sign = (spec[0] == '+') ? 1 : -1;
            const char *p = spec + 1;
            if (!(p[0] >= '0' && p[0] <= '9'))
                return false;
            int h = parse_int2(p);
            int m = 0;
            if (*p == ':')
            {
                ++p;
                m = parse_int2(p);
            }
            else if (*p >= '0' && *p <= '9')
            {
                m = parse_int2(p);
            }
            offset_minutes = sign * (h * 60 + m);
            name_out = nullptr;
            return true;
        }

        // Named timezone
        const TzEntry *entry = find_tz_entry(spec);
        if (entry)
        {
            offset_minutes = entry->utc_offset;
            name_out = entry->name;
            return true;
        }

        return false;
    }

    // Get the effective UTC offset for a named timezone at a given UTC timestamp.
    inline bool get_effective_offset(const char *tz_name, int64_t utc_ts, int &offset_minutes)
    {
        const TzEntry *entry = find_tz_entry(tz_name);
        if (!entry)
            return false;
        offset_minutes = effective_utc_offset(entry, utc_ts);
        return true;
    }

    // ============================================================================
    // 12. Timezone conversion
    // ============================================================================

    // Convert a DateTime to another timezone.
    //   - from_tz: parse the DateTime's own offset/name, or use its stored offset
    //   - to_tz_spec: target timezone name or offset string
    inline DateTime to_timezone(const DateTime &dt, const char *to_tz_spec)
    {
        int64_t ts = to_timestamp(dt); // absolute UTC moment

        int to_offset = 0;
        const char *to_name = nullptr;
        const TzEntry *to_entry = nullptr;

        if (to_tz_spec && *to_tz_spec)
        {
            // Try named timezone
            to_entry = find_tz_entry(to_tz_spec);
            if (to_entry)
            {
                to_offset = effective_utc_offset(to_entry, ts);
                to_name = to_entry->name;
            }
            else
            {
                // Try fixed offset
                if (!parse_timezone_offset(to_tz_spec, to_offset, to_name))
                {
                    to_offset = 0;
                    to_name = "UTC";
                }
            }
        }

        DateTime result = from_timestamp(ts, to_offset, to_name);
        return result;
    }

    inline DateTime to_utc(const DateTime &dt)
    {
        return to_timezone(dt, "UTC");
    }

    // Apply a named timezone (with DST) to a DateTime whose wall-clock time is already set.
    // Uses standard offset for an initial UTC approximation, then refines with DST.
    // Returns true on success, false if the timezone specifier is unrecognised.
    inline bool apply_timezone_to_datetime(DateTime &dt, const char *tz_spec)
    {
        const TzEntry *entry = find_tz_entry(tz_spec);
        if (entry)
        {
            dt.utc_offset_minutes = entry->utc_offset;
            dt.tz_name = entry->name;
            int64_t approx_ts = to_timestamp(dt);
            dt.utc_offset_minutes = effective_utc_offset(entry, approx_ts);
            return true;
        }
        int off = 0;
        const char *nm = nullptr;
        if (parse_timezone_offset(tz_spec, off, nm))
        {
            dt.utc_offset_minutes = off;
            dt.tz_name = (nm ? nm : TZ_EXPLICIT_OFFSET);
            return true;
        }
        return false;
    }

    // Internal: apply the global default timezone to a DateTime that carries
    // no explicit timezone.  Called by parse() / now() / from_unix().
    inline void _apply_default_timezone(DateTime &dt)
    {
        if (!_g_default_tz_name[0])
            return;
        if (dt.tz_name != nullptr)
            return; // already has explicit timezone
        const TzEntry *entry = find_tz_entry(_g_default_tz_name);
        if (entry)
        {
            dt.utc_offset_minutes = entry->utc_offset;
            dt.tz_name = entry->name;
            int64_t approx_ts = to_timestamp(dt);
            dt.utc_offset_minutes = effective_utc_offset(entry, approx_ts);
        }
        else
        {
            int off = 0;
            const char *nm = nullptr;
            parse_timezone_offset(_g_default_tz_name, off, nm);
            dt.utc_offset_minutes = off;
            dt.tz_name = (nm ? nm : TZ_EXPLICIT_OFFSET);
        }
    }

    // ============================================================================
    // 13. Current time
    // ============================================================================

    // Get current time as a DateTime in UTC.
    // Uses C standard library time(), which is lightweight and available everywhere.
    inline DateTime now_utc()
    {
        int64_t ts = (int64_t)::time(nullptr);
        return from_timestamp(ts, 0, "UTC");
    }

    // Get current time as a DateTime in a specific timezone.
    // If tz_spec is omitted and a global default is set, that default is used.
    // Otherwise returns UTC.
    inline DateTime now(const char *tz_spec = nullptr)
    {
        DateTime utc = now_utc();
        if (tz_spec && *tz_spec)
        {
            return to_timezone(utc, tz_spec);
        }
        if (_g_default_tz_name[0])
        {
            return to_timezone(utc, _g_default_tz_name);
        }
        return utc;
    }

    // ============================================================================
    // 14. List available timezones
    // ============================================================================

    inline std::vector<const char *> list_timezones()
    {
        std::vector<const char *> names;
        names.reserve(TZ_DB_SIZE);
        for (int i = 0; i < TZ_DB_SIZE; ++i)
        {
            names.push_back(TZ_DB[i].name);
        }
        return names;
    }

    // ============================================================================
    // 15. Convenience: timestamp <-> DateTime
    // ============================================================================

    // Convert a Unix timestamp to DateTime.  If tz_spec is omitted and a
    // global default is set, that default is used; otherwise UTC.
    inline DateTime from_unix(int64_t ts, const char *tz_spec = nullptr)
    {
        if (!tz_spec || !*tz_spec)
        {
            if (_g_default_tz_name[0])
                tz_spec = _g_default_tz_name;
            else
                tz_spec = "UTC";
        }
        int off = 0;
        const char *nm = nullptr;
        const TzEntry *entry = find_tz_entry(tz_spec);
        if (entry)
        {
            off = effective_utc_offset(entry, ts);
            nm = entry->name;
        }
        else
        {
            parse_timezone_offset(tz_spec, off, nm);
        }
        return from_timestamp(ts, off, nm);
    }


    // Convert a Unix timestamp to DateTime with the time set to 00:00:00 (midnight)
    // in the specified timezone.  Timezone logic matches from_unix.
    inline DateTime from_unix_date(int64_t ts, const char *tz_spec = nullptr)
    {
        DateTime dt = from_unix(ts, tz_spec);
        dt.time.hour = 0;
        dt.time.minute = 0;
        dt.time.second = 0;
        dt.time.nanosecond = 0;
        return dt;
    }

    inline int64_t to_unix(const DateTime &dt)
    {
        return to_timestamp(dt);
    }

    // ============================================================================
    // 16. Global default timezone
    // ============================================================================

    /// Set the global default timezone.  Accepts IANA names ("Asia/Shanghai")
    /// and fixed offsets ("+08:00", "-05:00").
    /// Throws std::invalid_argument if the timezone is not recognised.
    inline void set_default_timezone(const char *tz_spec)
    {
        if (!tz_spec || !*tz_spec)
        {
            std::snprintf(_g_default_tz_name, sizeof(_g_default_tz_name), "%s", "Asia/Shanghai");
            return;
        }
        const TzEntry *entry = find_tz_entry(tz_spec);
        if (entry)
        {
            std::snprintf(_g_default_tz_name, sizeof(_g_default_tz_name), "%s", tz_spec);
            return;
        }
        int off = 0;
        const char *nm = nullptr;
        if (parse_timezone_offset(tz_spec, off, nm))
        {
            std::snprintf(_g_default_tz_name, sizeof(_g_default_tz_name), "%s", tz_spec);
            return;
        }
        throw std::invalid_argument(
            std::string("chrono_util::set_default_timezone: unrecognised timezone '") + tz_spec + "'");
    }

    /// Return the current global default timezone name.
    inline const char *get_default_timezone()
    {
        return _g_default_tz_name[0] ? _g_default_tz_name : "Asia/Shanghai";
    }

    /// Check whether a timezone specifier is recognised without modifying state.
    inline bool is_valid_timezone(const char *tz_spec)
    {
        if (!tz_spec || !*tz_spec)
            return false;
        if (find_tz_entry(tz_spec))
            return true;
        int off = 0;
        const char *nm = nullptr;
        return parse_timezone_offset(tz_spec, off, nm);
    }

    /// Look up timezone database info.  Returns nullptr if not found.
    inline const TzEntry *get_timezone_info(const char *tz_spec)
    {
        if (!tz_spec || !*tz_spec)
            return nullptr;
        return find_tz_entry(tz_spec);
    }

} // namespace chrono_util