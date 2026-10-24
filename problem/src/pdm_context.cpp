/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pdm_context.h"

#include <atomic>
#include <format>
#include <random>

#include "c_chrono_util.h"
#include "c_log.h"
#include "scr_standard_csv_reader.h"

/**
 * @brief 生成 UUID v4，格式 xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
 */
std::string generate_uuid() {
  static thread_local std::mt19937_64 gen(std::random_device{}());
  const uint64_t hi = gen();
  const uint64_t lo = gen();

  // UUID v4：版本号固定为 4，variant 高 2 位固定为 10
  const auto time_low = static_cast<uint32_t>(hi >> 32);
  const auto time_mid = static_cast<uint32_t>((hi >> 16) & 0xFFFF);
  const auto time_hi = static_cast<uint32_t>(((hi >> 48) & 0x0FFF) | 0x4000);
  const auto clock_seq = static_cast<uint32_t>(((lo >> 48) & 0x3FFF) | 0x8000);
  const auto node_hi = static_cast<uint32_t>((lo >> 32) & 0xFFFF);
  const auto node_low = static_cast<uint32_t>(lo);

  return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:04x}{:08x}", time_low, time_mid, time_hi,
                     clock_seq, node_hi, node_low);
}  // namespace

/**
 * @brief 进程标识，取 uuid 的前 4 位，避免多个进程在同一秒内撞车
 */
const std::string& process_nonce() {
  static const std::string nonce = generate_uuid().substr(0, 4);
  return nonce;
}

/**
 * @brief 进程内的请求序号，避免同一进程并发求解时撞车
 */
uint64_t next_request_seq() {
  static std::atomic<uint64_t> seq{0};
  return seq.fetch_add(1) + 1;
}

/**
 * @brief 生成唯一的请求ID，格式 %Y%m%d%H%M%S_{进程标识}_{序号}
 * @details 时间戳保证可读可排序；进程标识与序号保证并发求解不会共用同一个输出目录和日志文件
 */
std::string generate_request_id() {
  return std::format("{}_{}_{}", chrono_util::format(chrono_util::now(), "%Y%m%d%H%M%S"),
                     process_nonce(), next_request_seq());
}

// ====== implement of SolverContext ======
SolverContext::SolverContext(std::string _root_dir, const std::string& log_dir,
                             const std::string& log_level)
    : root_dir(std::move(_root_dir)), request_id(generate_request_id()) {
  // 拼装输出目录
  this->output_dir = this->root_dir + "/output/" + this->request_id;
  // 创建本次请求专属的 logger：控制台 + log_dir + output_dir 三处输出
  // 必须先于场景读取，加载过程的日志才能落盘
  this->logger = LogManager::create(this->request_id, log_dir, this->output_dir, log_level);
  // 读取场景
  try {
    this->scenario =
        std::make_unique<StandardCsvReader>(this->root_dir, this->logger)->loading_scenario();
  } catch (const std::exception& e) {
    // common 层的工具函数拿不到 logger，失败原因在这里补记，保证三个输出里都有
    this->logger->error("读取场景失败: {}", e.what());
    throw;
  }
  // 读取参数
  this->parameter = load_parameter(this->root_dir, this->scenario, this->logger);
  // 可视化管理器
  this->visual_manager = new VisualManager(this->output_dir, this->scenario, this->logger);
  // 创建问题
  this->problem = new Problem(this->scenario, this->parameter);
}

SolverContext::~SolverContext() {
  // 日志刷盘，info/debug 不会留在缓冲区
  if (this->logger != nullptr) {
    this->logger->flush();
  }
  delete this->scenario;
  delete this->parameter;
  delete this->visual_manager;
  delete this->problem;
}
