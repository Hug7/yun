/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ctime>
#include <memory>
#include <algorithm>

#include "c_constant.h"

/**
 * @brief time window
 * @details contain the start time and end time of the time window
 */
class TimeWindow
{
public:
    /**
     * @brief start time of the time window
     */
    const long early;
    /**
     * @brief end time of the time window
     */
    const long late;

    TimeWindow() : early(TimeWindowParameter::DEFAULT_EARLY_TIME),
                   late(TimeWindowParameter::DEFAULT_LATE_TIME) {};

    TimeWindow(const long early, const long late) : early(early), late(late) {}
};