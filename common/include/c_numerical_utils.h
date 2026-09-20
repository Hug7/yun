/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cmath>

/**
 * @brief 精度的幂
 * @details 0:10^0:1, 1:10^1:10, 2:10^2:100, 3:10^3:1000, 4:10^4:10000
 */
constexpr int PRECISION_POW[5] = {1, 10, 100, 1000, 10000};

namespace NumUtil {
/**
 * @brief 保留小数点后几位
 * @param value 数值
 * @param precision 保留精度
 * @return
 */
inline double decimal_places(const double value, const int precision) {
  return std::round(value * PRECISION_POW[precision]) / PRECISION_POW[precision];
}
/**
 * @brief 将double数值放大
 * @param value 数值
 * @param precision 精度(10的指数)
 * @return value * 10^precision
 */
inline long scale_to_long(const double value, const int precision) {
  return static_cast<long>(value * PRECISION_POW[precision]);
}
/**
 * @brief 将long数值缩小
 * @details 缩小后的数值会保留小数点后precision位
 * @param value 数值
 * @param precision 精度(10的指数)
 * @return round(value / 10^precision, precision)
 */
inline double unscale_from_long(const long value, const int precision) {
  const double result = static_cast<double>(value) / static_cast<double>(PRECISION_POW[precision]);
  return decimal_places(result, precision);
}

}  // namespace
