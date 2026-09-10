/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <unordered_map>
#include <utility>

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
  PICK,
  DROP,
  START,
  END,
  NONE, // 必须放在最后，在部分场景用于计数，如应用在WorkEffect
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
  // unlimited
  NONE,
};

namespace Delimiter {
/**
 * @brief delimiter of calendar time range
 */
constexpr std::string CALENDAR_TIME_RANGE = ";";
/**
 * @brief delimiter of time window
 */
constexpr std::string TIME_WINDOW = ";";
}  // namespace Delimiter
namespace DistMatrixParameter {
/**
 * @brief maximum driving distance, unit: m
 * @details representative 6000 km
 */
constexpr long MAX_DISTANCE = 6000000;
/**
 * @brief maximum driving time, unit: s
 * @details representative 100 h
 */
constexpr long MAX_TIME = 360000;
}  // namespace DistMatrixParameter

namespace LocationParameter {
/**
 * @brief default location code
 */
constexpr std::string DEFAULT_LOCATION_CODE = "default";
}  // namespace LocationParameter

namespace WorkPlanParameter {
/**
 * @brief default calendar time range, eq. [00:00, 23:59]
 */
constexpr std::pair<int, int> DEFAULT_CALENDAR_TIME_RANGE = std::make_pair(0, 86340);
/**
 * @brief default work effect quantity
 */
constexpr double DEFAULT_WORK_EFFECT_QUANTITY = 0.0;
/**
 * @brief precision of work effect quantity
 */
constexpr int WORK_EFFECT_QUANTITY_PRECISION = 4;
}  // namespace WorkPlanParameter

namespace TimeWindowParameter {
/**
 * @brief default paln datetime range (start), unit: s
 * @details representative 1970-01-01 00:00
 */
constexpr long DEFAULT_PLAN_DATETIME_RANGE_START = -28800;
/**
 * @brief default paln datetime range (end), unit: s
 * @details representative 2096-11-16 02:00
 */
constexpr long DEFAULT_PLAN_DATETIME_RANGE_END = 4003840810;
/**
 * @brief default early time, unit: s
 * @details representative 1970-01-01 00:00
 */
constexpr long DEFAULT_EARLY_TIME = -28800;
/**
 * @brief default early time, unit: s
 * @details representative 2096-11-16 02:00
 */
constexpr long DEFAULT_LATE_TIME = 4003840810;
/**
 * @brief max interval of continuous time bucket, unit: s
 */
constexpr int INTERVAL_SECS_CONTINUOUS_TIME_BUCKET = 600;
}  // namespace TimeWindowParameter

namespace HardConstraintParameter {
/**
 * @brief default hard constraint: max number of pick nodes in load
 */
constexpr int DEFAULT_MAX_PICK_NODE_COUNT = 0;
/**
 * @brief default hard constraint: max number of drop nodes in load
 */
constexpr int DEFAULT_MAX_DROP_NODE_COUNT = 0;

}  // namespace HardConstraintParameter

namespace SoftConstraintParameter {
/**
 * @brief cost constraint "sc_dist" default distance factor
 * @details enable constraint if greater than 0
 */
constexpr int SC_DIST_DEFAULT_DIST_FACTOR = 0;

}  // namespace CostConstraintParameter

namespace CostConstraintParameter {
/**
 * @brief cost constraint "cc_dist" default distance factor
 * @details enable constraint if greater than 0
 */
constexpr int CC_DIST_DEFAULT_DIST_FACTOR = 0;

}  // namespace CostConstraintParameter

namespace LoadParameter {
/**
 * @brief default load object value
 */
constexpr double INIT_LOAD_OBJ_VAL = 1000000000.0;
}  // namespace LoadParameter
