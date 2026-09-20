/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief 日志管理器
 * @details 为每个请求创建一个独立的 logger，三个输出：控制台、log_dir、output_dir。
 *          全程不碰 spdlog 的全局默认 logger，因此多个请求可以并发创建、并发写日志。
 *          logger 句柄由 SolverContext 持有，需要打日志的模块通过它拿到句柄。
 */
namespace LogManager {

/**
 * @brief 日志格式：时间(毫秒) + 线程号 + 等级 + 内容
 */
inline constexpr auto PATTERN = "[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %v";

namespace detail {

/**
 * @brief 日志文件后缀
 */
inline constexpr const char* SUFFIX = ".log";

/**
 * @brief 所有请求共享的控制台 sink
 * @details 共享同一个对象，多个请求并发写控制台时才会被 sink 内部的锁串行化
 */
inline const spdlog::sink_ptr& console_sink() {
  static const spdlog::sink_ptr sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  return sink;
}

/**
 * @brief 解析日志等级
 * @details spdlog 对认不出的名字返回 off（等于静默关掉全部日志），这里退化为 info
 */
inline spdlog::level::level_enum parse_level(const std::string& level) {
  std::string name = level;
  std::transform(name.begin(), name.end(), name.begin(),
                 [](const unsigned char c) { return std::tolower(c); });
  const auto parsed = spdlog::level::from_str(name);
  if (parsed == spdlog::level::off && name != "off") {
    std::cerr << "[LogManager] 无法识别的日志等级 '" << level << "'，退化为 info" << std::endl;
    return spdlog::level::info;
  }
  return parsed;
}

/**
 * @brief 追加一个文件 sink
 * @details 目录不可写时只告警不抛异常：日志失败不能中断求解
 */
inline void add_file_sink(std::vector<spdlog::sink_ptr>& sinks, const std::string& dir,
                          const std::string& request_id) {
  if (dir.empty()) {
    return;
  }
  const std::string file_path = dir + "/" + request_id + SUFFIX;
  try {
    // 追加模式：同一 request_id 重复求解时不丢上一次的日志（父目录由 spdlog 自动创建）
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_path, false));
  } catch (const spdlog::spdlog_ex& ex) {
    std::cerr << "[LogManager] " << file_path << " 无法写入: " << ex.what() << std::endl;
  }
}

}  // namespace detail

/**
 * @brief 为一次请求创建 logger
 * @param request_id 请求ID，决定两个目录下日志文件的文件名，同时作为 logger 的名字
 * @param log_dir 全局日志目录，为空则不产生该输出
 * @param output_dir 本次请求的输出目录，为空则不产生该输出
 * @param level 日志等级，三个输出共用
 * @return 该请求专属的 logger，由调用方持有并在析构时 flush
 */
inline std::shared_ptr<spdlog::logger> create(const std::string& request_id,
                                              const std::string& log_dir,
                                              const std::string& output_dir,
                                              const std::string& level) {
  std::vector<spdlog::sink_ptr> sinks{detail::console_sink()};
  detail::add_file_sink(sinks, log_dir, request_id);
  detail::add_file_sink(sinks, output_dir, request_id);

  // 不注册进 spdlog 的 registry：request_id 撞车时不会抛 logger already exists
  auto logger = std::make_shared<spdlog::logger>(request_id, sinks.begin(), sinks.end());
  logger->set_pattern(PATTERN);
  logger->set_level(detail::parse_level(level));
  logger->flush_on(spdlog::level::warn);
  logger->info("Logger created. request_id={}, level={}, log_dir={}, output_dir={}", request_id,
               level, log_dir, output_dir);
  return logger;
}

}  // namespace LogManager
