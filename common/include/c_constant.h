/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace Delimiter
{
    /**
     * @brief delimiter of calendar time range 
     */
    inline constexpr std::string CALENDAR_TIME_RANGE = ";";
    /**
     * @brief delimiter of time window
     */
    inline constexpr std::string TIME_WINDOW = ";";
}

enum class CalendarType
{
    PICK,
    DROP,
    RESTRICT,
};

static inline const std::unordered_map<std::string, CalendarType> CalendarTypeMap = {
    {"PICK", CalendarType::PICK},
    {"DROP", CalendarType::DROP},
    {"RESTRICT", CalendarType::RESTRICT},
};

enum class CalendarTimeRangeType
{
    DAILY,
    WEEKLY,
};

enum class ActivityType
{
    NONE,
    PICK,
    DROP,
    START,
    END,
};

static inline const std::unordered_map<std::string, ActivityType> ActivityTypeMap = {
    {"PICK", ActivityType::PICK},
    {"DROP", ActivityType::DROP},
};

namespace DistMatrixParameter
{
    /**
     * @brief maximum driving distance, unit: m
     * @details representative 6000 km
     */
    long const MAX_DISTANCE = 6000000;
    /**
     * @brief maximum driving time, unit: s
     * @details representative 100 h
     */
    long const MAX_TIME = 360000;
}

namespace LocationParameter
{
    /**
     * @brief default location code
     */
    std::string const DEFAULT_LOCATION_CODE = "default";
}

namespace TimeWindowParameter
{
    /**
     * @brief default early time, unit: s
     * @details representative 1970-01-01 00:00:00
     */
    long const DEFAULT_EARLY_TIME = -28800;
    /**
     * @brief default early time, unit: s
     * @details representative 2096-11-16 02:00:00
     */
    long const DEFAULT_LATE_TIME = 4003840810;
    /**
     * @brief max interval of continuous time bucket, unit: s
     */
    int const INTERVAL_SECS_CONTINUOUS_TIME_BUCKET = 600;
}

enum class PickDropPatternType
{
    // pick first and last drop, single pick and multi drop
    SPMD,
    // pick first and last drop, multi pick and multi drop
    MPMD,
    // pickup and delivery
    PDP,
};
