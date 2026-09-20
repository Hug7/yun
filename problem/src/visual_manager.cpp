/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visual_manager.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <string>

// ====== implement of Load VisualManager ======
VisualManager::VisualManager(std::string output_dir, const Scenario* scenario,
                             const std::shared_ptr<spdlog::logger>& logger)
    : output_dir(std::move(output_dir)), scenario(scenario), logger(logger) {
  if (!std::filesystem::exists(this->output_dir)) {
    try {
      std::filesystem::create_directories(this->output_dir);
    } catch (const std::exception& e) {
      this->logger->error(std::format("创建文件夹 {} 失败!", this->output_dir));
      this->logger->error(e.what());
    }
  }
}

std::string VisualManager::resolve_target_dir(const std::string& target_dir) const {
  if (target_dir.empty()) {
    return this->output_dir;
  }
  const std::string target_dir_path = this->output_dir + "/" + target_dir;
  // target_dir不存在则创建
  if (!std::filesystem::exists(target_dir_path)) {
    try {
      std::filesystem::create_directories(target_dir_path);
    } catch (const std::exception& e) {
      this->logger->error(std::format("创建文件夹 {} 失败!", target_dir_path));
      this->logger->error(e.what());
    }
  }
  return target_dir_path;
}
