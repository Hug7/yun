/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visual_manager.h"

#include <gtest/gtest.h>

#include <filesystem>
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

  void TearDown() override {
    std::filesystem::permissions(kRoot, std::filesystem::perms::owner_all);
    std::filesystem::remove_all(kRoot);
  }
};

TEST_F(VisualManagerTest, ConstructorThrowsWhenOutputDirIsNotWritable) {
  const auto readonly_dir = kRoot / "readonly";
  std::filesystem::create_directories(readonly_dir);
  std::filesystem::permissions(readonly_dir, std::filesystem::perms::owner_read |
                                                 std::filesystem::perms::owner_exec);
  const auto logger = LogManager::create("visual_manager_test", "", "", "error");

  EXPECT_THROW(VisualManager((readonly_dir / "output").string(), nullptr, logger),
               std::runtime_error);
}

TEST_F(VisualManagerTest, WriteResultThrowsWhenOutputDirIsNotWritable) {
  const auto readonly_dir = kRoot / "readonly";
  std::filesystem::create_directories(readonly_dir);
  std::filesystem::permissions(readonly_dir, std::filesystem::perms::owner_read |
                                                 std::filesystem::perms::owner_exec);
  const auto logger = LogManager::create("visual_manager_test", "", "", "error");
  const VisualManager manager(readonly_dir.string(), nullptr, logger);

  EXPECT_THROW(manager.visual_load_to_json({}, readonly_dir.string()), std::runtime_error);
}

}  // namespace
