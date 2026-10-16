/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <format>
#include <string>
#include <utility>

/**
 * @brief 自定义 yaml node
 */
class YamlNode {
 public:
  YAML::Node root_;

  const std::string path_;

  explicit YamlNode(const YAML::Node& root) : root_(root) {}

  explicit YamlNode(const YAML::Node& root, std::string path)
      : root_(root), path_(std::move(path)) {}

  YamlNode() = default;

  bool contains_key(const std::string& key) const {
    if (!this->root_ || this->root_[key]) {
      return true;
    }
    return false;
  }

  int parse_int(const std::string& key) const {
    const auto node = this->get_node(key);
    try {
      return node.as<int>();
    } catch (const YAML::Exception&) {
      fail(this->sub_path(key), "数据类型不符合预期`int`");
    }
    return -1;
  }

  double parse_double(const std::string& key) const {
    const auto node = this->get_node(key);
    try {
      return node.as<double>();
    } catch (const YAML::Exception&) {
      fail(this->sub_path(key), "数据类型不符合预期`double`");
    }
    return -1;
  }

  std::string parse_string(const std::string& key) const {
    const auto node = this->get_node(key);
    try {
      return node.as<std::string>();
    } catch (const YAML::Exception&) {
      fail(this->sub_path(key), "数据类型不符合预期`string`");
    }
    return "";
  }

  YAML::Node get_node(const std::string& key) const {
    if (!this->root_ || !this->root_[key]) {
      fail(this->sub_path(key), "参数不存在");
    }
    return this->root_[key];
  }

  YamlNode get_yaml_node(const std::string& key) const {
    if (!this->root_ || !this->root_[key]) {
      fail(this->sub_path(key), "参数不存在");
    }
    return YamlNode(this->root_[key], this->sub_path(key));
  }

  YamlNode parse_map(const std::string& key) const {
    const auto node = this->get_node(key);
    if (!node.IsMap()) {
      fail(this->sub_path(key), "数据类型不符合预期`map`");
    }
    return YamlNode(node, this->sub_path(key));
  }

  YamlNode parse_list(const std::string& key) const {
    const auto node = this->get_node(key);
    if (!node.IsSequence()) {
      fail(this->sub_path(key), "数据类型不符合预期`list`");
    }
    return YamlNode(node, this->sub_path(key));
  }

  void fail(const std::string& path, const std::string& reason) const {
    throw std::runtime_error(std::format("{}: {}", path, reason));
  }

 private:
  std::string sub_path(const std::string& key) const {
    return std::format("{}.{}", this->path_, key);
  }
};

/**
 * @brief 读取yaml参数
 * @param file_path 文件
 * @param logger logger
 * @return yaml参数
 */
inline YamlNode read_yaml(const std::string& file_path,
                          const std::shared_ptr<spdlog::logger>& logger) {
  if (!std::filesystem::exists(file_path)) {
    logger->error("File {} does not exist.", file_path);
    throw std::filesystem::filesystem_error(
        file_path, std::error_code(static_cast<int>(std::errc::no_such_file_or_directory),
                                   std::generic_category()));
  }

  YAML::Node root;
  try {
    root = YAML::LoadFile(file_path);
  } catch (const YAML::Exception& e) {
    logger->error(std::format("{} 解析失败: {}", file_path, e.what()));
    throw std::runtime_error(std::format("{} 解析失败: {}", file_path, e.what()));
  }
  if (!root.IsMap()) {
    logger->error(std::format("{} 不是字典结构", file_path));
    throw std::runtime_error(std::format("{} 不是字典结构", file_path));
  }

  return YamlNode(root);
}
