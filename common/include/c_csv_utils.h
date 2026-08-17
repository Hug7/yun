/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>

#include "c_rapidcsv.h"

namespace CsvUtils {
/**
 * @brief 检测csv文件中的列是否存在
 * @tparam N
 * @param doc
 * @param headers
 * @param filename
 */
template <size_t N>
void check_column_exist(rapidcsv::Document& doc, const std::array<const char*, N>& headers,
                        const char* filename) {
  for (const auto& h : headers) {
    if (doc.GetColumnIdx(h) == -1) {
      throw std::runtime_error(std::string(filename) + " file missing column: " + h);
    }
  }
}
}  // namespace CsvUtils