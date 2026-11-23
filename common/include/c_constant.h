/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
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
  NONE,  // 必须放在最后，在部分场景用于计数，如应用在WorkEffect
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
inline const std::string CALENDAR_TIME_RANGE = ";";
/**
 * @brief delimiter of time window
 */
inline const std::string TIME_WINDOW = ";";
}  // namespace Delimiter

namespace DistMatrixParameter {
/**
 * @brief maximum driving distance, unit: m
 * @details representative 60000 km
 */
constexpr std::int64_t MAX_DISTANCE = 60000000;
/**
 * @brief maximum driving time, unit: s
 * @details representative 1000 h
 */
constexpr std::int64_t MAX_TIME = 3600000;
}  // namespace DistMatrixParameter

namespace LocationParameter {
/**
 * @brief default location code
 */
inline const std::string DEFAULT_LOCATION_CODE = "default";
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
 * @brief default plan datetime range (start), unit: s
 * @details representative 1970-01-01 00:00
 */
constexpr std::int64_t DEFAULT_PLAN_DATETIME_RANGE_START = -28800;
/**
 * @brief default paln datetime range (end), unit: s
 * @details representative 2096-11-16 02:00
 */
constexpr std::int64_t DEFAULT_PLAN_DATETIME_RANGE_END = 4003840810;
/**
 * @brief default early time, unit: s
 * @details representative 1970-01-01 00:00
 */
constexpr std::int64_t DEFAULT_EARLY_TIME = -28800;
/**
 * @brief default early time, unit: s
 * @details representative 2096-11-16 02:00
 */
constexpr std::int64_t DEFAULT_LATE_TIME = 4003840810;
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

}  // namespace SoftConstraintParameter

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

namespace CacheParameter {
/**
 * @brief 构造时预分配的桶位数, 与容量上限无关(上限见 PICK_DROP_MAX_ENTRY_COUNT)
 */
static constexpr int PICK_DROP_RESERVED_ENTRY_COUNT = 50000;
/**
 * @brief 时间窗冻结表 pick/drop 的条目上限, 达到后按水位回收
 * @details 键已收窄为时间窗模式 id 集合, 条目数只与时间窗内容组合相关; 该上限仅兜底
 * 退化输入, 保证任何算例下内存都有硬边界。回收只会让后续查询重新计算, 不影响结果。
 */
static constexpr int PICK_DROP_MAX_ENTRY_COUNT = 50000;
/**
 * @brief 达到上限后回收到的目标条目数(上限的 3/4)
 * @details 每次只淘汰约 1/4, 而不是整表清空: 热点条目大概率保留, 重算被摊薄到多轮回收,
 * 避免清空瞬间集中重算造成的耗时毛刺。目标值必须小于上限。
 */
static constexpr int PICK_DROP_RECYCLE_TARGET_COUNT = 37500;
}  // namespace CacheParameter
