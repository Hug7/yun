/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "c_chrono_util.h"

/**
 * @brief 函数计时
 */
class FuncTiming {
 public:
  using UPtr = std::unique_ptr<FuncTiming>;
  using VecUPtr = std::vector<UPtr>;
  /**
   * @brief 函数编码
   */
  std::string func_code;
  /**
   * @brief 开始时间
   */
  std::string start_date_time;
  /**
   * @brief 开始时间戳(s)
   */
  long start_date_time_sec{0};
  /**
   * @brief 结束时间
   */
  std::string end_date_time;
  /**
   * @brief 结束时间戳(s)
   */
  long end_date_time_sec{0};

  explicit FuncTiming(std::string func_code) : func_code{std::move(func_code)} {
    const auto date_time = chrono_util::now();
    this->start_date_time = chrono_util::format(date_time, "%Y-%m-%d %H:%M:%S");
    this->start_date_time_sec = chrono_util::to_timestamp(date_time);
  }

  void end_timing() {
    const auto date_time = chrono_util::now();
    this->end_date_time = chrono_util::format(date_time, "%Y-%m-%d %H:%M:%S");
    this->end_date_time_sec = chrono_util::to_timestamp(date_time);
  }

  [[nodiscard]] bool is_not_complete() const { return this->end_date_time.empty(); }

  [[nodiscard]] long elapsed_second() const {
    return this->end_date_time_sec - this->start_date_time_sec;
  }
};
