/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "c_log.h"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kThreads = 8;
constexpr int kLines = 200;

const std::filesystem::path kRoot = std::filesystem::temp_directory_path() / "yun_c_log_test";

std::vector<std::string> ReadLines(const std::filesystem::path& file) {
  std::vector<std::string> lines;
  std::ifstream in(file);
  std::string line;
  while (std::getline(in, line)) {
    lines.push_back(line);
  }
  return lines;
}

/**
 @brief 断言 line 属于 request_id 这个请求
 */
void ExpectBelongsTo(const std::string& line, const std::string& request_id) {
  const bool mine = line.find("tag" + request_id) != std::string::npos ||
                    line.find("request_id=" + request_id) != std::string::npos;
  EXPECT_TRUE(mine) << "串档: " << line;
}

}  // namespace

class LogManagerTest : public ::testing::Test {
 protected:
  void SetUp() override { std::filesystem::remove_all(kRoot); }

  void TearDown() override { std::filesystem::remove_all(kRoot); }
};

/**
 * @brief 三个输出：控制台 + log_dir + output_dir，两个文件名都是 {request_id}.log
 */
TEST_F(LogManagerTest, WritesBothFiles) {
  const std::string log_dir = (kRoot / "log").string();
  const std::string output_dir = (kRoot / "output").string();
  const auto logger = LogManager::create("req_a", log_dir, output_dir, "debug");
  logger->debug("hello {}", 42);
  logger->flush();

  for (const auto& dir : {log_dir, output_dir}) {
    const auto lines = ReadLines(std::filesystem::path(dir) / "req_a.log");
    // 建 logger 时的那条 + hello 42
    ASSERT_EQ(lines.size(), 2U);
    EXPECT_NE(lines[1].find("[debug]"), std::string::npos);
    EXPECT_NE(lines[1].find("hello 42"), std::string::npos);
  }
}

/**
 * @brief 目录为空则跳过该输出，目录不可写也不能抛异常
 */
TEST_F(LogManagerTest, EmptyDirIsSkipped) {
  const auto logger = LogManager::create("req_b", "", "", "info");
  ASSERT_NE(logger, nullptr);
  EXPECT_EQ(logger->sinks().size(), 1U);
}

/**
 * @brief 不许碰全局默认 logger，否则并发写日志会 use-after-free
 */
TEST_F(LogManagerTest, KeepsDefaultLoggerUntouched) {
  const auto* before = spdlog::default_logger_raw();
  const auto logger = LogManager::create("req_c", (kRoot / "log").string(), "", "info");
  ASSERT_NE(logger, nullptr);
  EXPECT_EQ(spdlog::default_logger_raw(), before);
}

/**
 * @brief 等级无法识别时退化为 info，而不是 spdlog 的 off（等于关掉全部日志）
 */
TEST_F(LogManagerTest, UnparsableLevelFallsBackToInfo) {
  EXPECT_EQ(LogManager::create("req_d", "", "", "nonsense")->level(), spdlog::level::info);
  EXPECT_EQ(LogManager::create("req_e", "", "", "DEBUG")->level(), spdlog::level::debug);
  EXPECT_EQ(LogManager::create("req_f", "", "", "off")->level(), spdlog::level::off);
}

/**
 * @brief 多线程并发创建 logger 并写日志，各请求的日志互不串档
 */
TEST_F(LogManagerTest, ConcurrentRequestsDoNotCrossWrite) {
  std::atomic<bool> go{false};
  std::vector<std::thread> threads;
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([&go, i] {
      // 同时起跑，让 create 真的撞上
      while (!go.load()) {
      }
      const std::string request_id = "req" + std::to_string(i);
      const auto logger = LogManager::create(request_id, (kRoot / "log").string(),
                                             (kRoot / "out" / request_id).string(), "debug");
      for (int j = 0; j < kLines; ++j) {
        logger->info("tag{} payload {}", request_id, j);
      }
      logger->flush();
    });
  }
  go.store(true);
  for (auto& thread : threads) {
    thread.join();
  }

  for (int i = 0; i < kThreads; ++i) {
    const std::string request_id = "req" + std::to_string(i);
    const auto lines = ReadLines(kRoot / "log" / (request_id + ".log"));
    ASSERT_EQ(lines.size(), kLines + 1);
    for (const auto& line : lines) {
      ExpectBelongsTo(line, request_id);
    }
  }
}
