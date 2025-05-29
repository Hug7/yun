#ifndef LOG_H
#define LOG_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <yaml-cpp/yaml.h>
#include <memory>
#include <iostream>

struct LoggerConfig
{
    std::string log_level;
    std::string log_file;
    std::string prefix;
    std::string pattern;

    LoggerConfig() : log_level("info"), log_file(""), prefix(""), pattern("[%Y-%m-%d %H:%M:%S] [%l] %v") {}
};

void init_logger(std::string log_level, std::string log_file, std::string prefix, std::string pattern)
{
    try
    {
        // 控制台接收器
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        // 文件接收器
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
        // 创建多接收器的日志记录器
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>(prefix, sinks.begin(), sinks.end());
        // // 设置日志格式
        // if (pattern != "")
        // {
        //     logger->set_pattern(pattern);
        // }
        // 设置日志级别
        if (log_level == "info")
        {
            logger->set_level(spdlog::level::info);
        }
        else if (log_level == "debug")
        {
            logger->set_level(spdlog::level::debug);
        }
        else if (log_level == "warn")
        {
            logger->set_level(spdlog::level::warn);
        }
        else if (log_level == "error")
        {
            logger->set_level(spdlog::level::err);
        }
        else if (log_level == "off")
        {
            logger->set_level(spdlog::level::off);
        }
        else
        {
            logger->set_level(spdlog::level::info);
        }

        // 设置为全局默认日志记录器
        spdlog::set_default_logger(logger);
        spdlog::info("Log initialized!");
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    }
}

void init_logger(LoggerConfig &logger_config)
{
    init_logger(logger_config.log_level, logger_config.log_file, logger_config.prefix, logger_config.pattern);
}

void init_logger_by_yaml(std::string config_file_path)
{
    LoggerConfig logger_config;
    try
    {
        // 加载 YAML 配置文件
        YAML::Node config = YAML::LoadFile("/Users/liq433/workspace/code/wx/wx_route_plan/Config.yaml");
        YAML::Node log_node = config["log"];
        if (log_node)
        {
            // 获取日志级别
            if (log_node["level"])
            {
                logger_config.log_level = log_node["level"].as<std::string>();
            }
        }
        // 获取日志路径
        if (log_node["file"])
        {
            logger_config.log_file = log_node["file"].as<std::string>();
        }
        // 获取日志前缀
        if (log_node["prefix"])
        {
            logger_config.prefix = log_node["prefix"].as<std::string>();
        }
        // 获取日志格式
        if (log_node["pattern"])
        {
            logger_config.pattern = log_node["pattern"].as<std::string>();
        }
    }
    catch (const YAML::Exception &e)
    {
        std::cerr << "Error loading or parsing YAML file: " << e.what() << std::endl;
        throw e;
    }
    init_logger(logger_config);
}

#endif // LOG_H
