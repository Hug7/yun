/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <iostream>
#include <filesystem>

namespace FileUtils
{
    inline bool file_exists(const std::string &file_path)
    {
        return std::filesystem::exists(file_path);
    }

    inline bool file_exists(const std::string &file_path, const std::string &file_name, const bool file_must_exist)
    {
        if (!file_exists(file_path))
        {
            if (file_must_exist)
            {
                spdlog::error("File {} does not exist.", file_name);
                throw std::filesystem::filesystem_error(
                    file_path,
                    std::error_code(static_cast<int>(std::errc::no_such_file_or_directory), std::generic_category()));
            }
            return false;
        }
        return true;
    }

    inline bool file_exists(const std::string &file_path, const std::string &file_name)
    {
        return file_exists(file_path, file_name, true);
    }
}