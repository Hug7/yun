/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

namespace StringUtil {
/**
 * @brief Splits a string into tokens based on a delimiter.
 * @param str The string to split.
 * @param delimiter The delimiter to split the string on.
 */
std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
  std::vector<std::string> tokens;

  if (str.empty() || delimiter.empty()) {
    if (delimiter.empty()) {
      for (char ch : str) {
        tokens.emplace_back(1, ch);
      }
    }
    return tokens;
  }

  size_t start = 0;
  size_t end = str.find(delimiter);
  while (end != std::string::npos) {
    tokens.push_back(str.substr(start, end - start));
    start = end + delimiter.length();
    end = str.find(delimiter, start);
  }
  tokens.emplace_back(str.substr(start));

  return tokens;
}
}  // namespace StringUtil