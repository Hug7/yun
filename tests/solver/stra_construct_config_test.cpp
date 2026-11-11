/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stra_construct_config.h"

#include <gtest/gtest.h>

#include <sol/sol.hpp>
#include <stdexcept>

namespace {

/**
 * @brief construct_knn Lua→POD 解析测试夹具
 */
class StraConstructConfigTest : public ::testing::Test {
 protected:
  void SetUp() override { this->lua.open_libraries(sol::lib::base, sol::lib::table); }

  sol::state lua;
};

}  // namespace

TEST_F(StraConstructConfigTest, MissingParameterKeepsDefaults) {
  const KnnParameter parameter = parse_construct_knn_parameter(sol::object{});

  const KnnParameter default_parameter{};
  EXPECT_EQ(parameter.neighbor_count, default_parameter.neighbor_count);
}

TEST_F(StraConstructConfigTest, NilParameterKeepsDefaults) {
  const sol::object nil_value = this->lua.script("return nil").get<sol::object>();

  const KnnParameter parameter = parse_construct_knn_parameter(nil_value);

  const KnnParameter default_parameter{};
  EXPECT_EQ(parameter.neighbor_count, default_parameter.neighbor_count);
}

TEST_F(StraConstructConfigTest, EmptyTableKeepsDefaults) {
  const sol::object cfg = this->lua.script("return {}").get<sol::object>();

  const KnnParameter parameter = parse_construct_knn_parameter(cfg);

  const KnnParameter default_parameter{};
  EXPECT_EQ(parameter.neighbor_count, default_parameter.neighbor_count);
}

TEST_F(StraConstructConfigTest, NeighborCountIsConvertedToPod) {
  const sol::object cfg = this->lua.script("return { neighbor_count = 12 }").get<sol::object>();

  const KnnParameter parameter = parse_construct_knn_parameter(cfg);

  EXPECT_EQ(parameter.neighbor_count, 12);
}

TEST_F(StraConstructConfigTest, NonTableParameterThrows) {
  const sol::object cfg = this->lua.script("return 12").get<sol::object>();

  EXPECT_THROW(parse_construct_knn_parameter(cfg), std::runtime_error);
}
