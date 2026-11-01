/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visual_manager.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "c_log.h"

namespace {

const std::filesystem::path kRoot =
    std::filesystem::temp_directory_path() / "yun_visual_manager_test";

class VisualManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::filesystem::remove_all(kRoot);
    std::filesystem::create_directories(kRoot);
  }

  void TearDown() override { std::filesystem::remove_all(kRoot); }
};

TEST_F(VisualManagerTest, ConstructorThrowsWhenOutputParentIsFile) {
  const auto blocker = kRoot / "blocker";
  std::ofstream(blocker).put('x');
  const auto logger = LogManager::create("visual_manager_test", "", "", "error");

  EXPECT_THROW(VisualManager((blocker / "output").string(), nullptr, logger), std::runtime_error);
}

TEST_F(VisualManagerTest, WriteResultThrowsWhenTargetIsFile) {
  const auto blocker = kRoot / "blocker2";
  std::ofstream(blocker).put('x');
  const auto logger = LogManager::create("visual_manager_test", "", "", "error");
  const VisualManager manager((kRoot / "out").string(), nullptr, logger);

  EXPECT_THROW(manager.visual_load_to_json({}, blocker.string()), std::runtime_error);
}

}  // namespace
