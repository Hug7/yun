/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <regex>
#include <format>

#include "c_chrono_util.h"

namespace TimeFromatted
{
    static const std::regex pattern_hhmm("^(?:[01][0-9]|2[0-3])[0-5][0-9]$");

    static const std::regex pattern_hhmmhhmm("^(?:[01][0-9]|2[0-3])[0-5][0-9](?:[01][0-9]|2[0-3])[0-5][0-9]$");

    inline void check_hhmm(const std::string &str)
    {
        if (!std::regex_match(str, pattern_hhmm))
        {
            throw std::invalid_argument(std::format("{} is invalid time format, should be hhmm", str));
        }
    }

    inline void check_hhmmhhmm(const std::string &str)
    {
        if (!std::regex_match(str, pattern_hhmmhhmm))
        {
            throw std::invalid_argument(std::format("{} is invalid time format, should be hhmmhhmm", str));
        }
    }

    inline int hhmm_parse_time_sec(const std::string &str)
    {
        check_hhmm(str);

        int hour = std::stoi(str.substr(0, 2));
        int min = std::stoi(str.substr(2, 2));

        return hour * 3600 + min * 60;
    }

    inline std::pair<int, int> hhmmhhmm_parse_time_range(const std::string &str)
    {
        int start = hhmm_parse_time_sec(str.substr(0, 4));
        int end = hhmm_parse_time_sec(str.substr(4, 4));

        if (start > end)
        {
            throw std::invalid_argument(std::format("{} is invalid time format, start time should be less than end time", str));
        }

        return std::make_pair(start, end);
    }
}

namespace TimeParse
{
    static const char *fmt_yyyymmddhhmm_1 = "%Y-%m-%d %H:%M";

    static const char *fmt_yyyymmddhhmm_2 = "%Y%m%d%H%M";

    inline long parse_tm_to_sec(
        const std::string &time_str,
        const char *fmt)
    {
        chrono_util::DateTime dt = chrono_util::parse(time_str, fmt);
        return static_cast<long>(chrono_util::to_unix(dt));
    }
}
