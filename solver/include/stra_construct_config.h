/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sol/sol.hpp>

#include "algo_construct.h"

/**
 * @brief 解析 Lua 中 `construct_knn` 的参数
 * @param parameter lua 传给 `construct_knn` 的参数, 允许 nil
 * @return knn 构造的结构化参数
 * @details 只负责 Lua 语法与形状: 键名与类型读取, 缺省字段由 `KnnParameter` 的默认值补齐
 */
KnnParameter parse_construct_knn_parameter(const sol::object& parameter);
