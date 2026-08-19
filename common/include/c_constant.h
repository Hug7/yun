/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <unordered_map>

enum class CalendarType {
  PICK,
  DROP,
  RESTRICT,
};

static inline const std::unordered_map<std::string, CalendarType> CalendarTypeMap = {
    {"PICK", CalendarType::PICK},
    {"DROP", CalendarType::DROP},
    {"RESTRICT", CalendarType::RESTRICT},
};

enum class CalendarTimeRangeType {
  DAILY,
  WEEKLY,
};

enum class ActivityType {
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

enum class PickDropPatternType {
  // single pick location and single drop location
  SPSD,
  // single pick location and multi drop locations
  SPMD,
  // multi pick locations and multi drop locations
  MPMD,
  // multi pick locations and single drop locations
  MPSD,
};

enum class LoadUnloadPolicyType {
  // first pick last drop
  FILO,
  // first pick last drop
  FIFO,
  // umlimited
  NONE,
};

namespace Delimiter {
/**
 * @brief delimiter of calendar time range
 */
inline constexpr std::string CALENDAR_TIME_RANGE = ";";
/**
 * @brief delimiter of time window
 */
inline constexpr std::string TIME_WINDOW = ";";
}  // namespace Delimiter
namespace DistMatrixParameter {
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
}  // namespace DistMatrixParameter

namespace LocationParameter {
/**
 * @brief default location code
 */
std::string const DEFAULT_LOCATION_CODE = "default";
}  // namespace LocationParameter

namespace TimeWindowParameter {
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
}  // namespace TimeWindowParameter

namespace HardConstraintParameter {
/**
 * @brief default hard constraint: max number of pick nodes in load
 */
int const DEFAULT_MAX_PICK_NODE_COUNT = 0;
/**
 * @brief default hard constraint: max number of drop nodes in load
 */
int const DEFAULT_MAX_DROP_NODE_COUNT = 0;

}  // namespace HardConstraintParameter

namespace CostConstraintParameter {
/**
 * @brief cost constraint "cc_dist" default distance factor
 * @details enable constraint if greater than 0
 */
int const CC_DIST_DEFAULT_DIST_FACTOR = 0;

}  // namespace CostConstraintParameter

namespace LoadParameter {
/**
 * @brief default load object value
 */
double const INIT_LOAD_OBJ_VAL = 1000000000.0;
}  // namespace LoadParameter
