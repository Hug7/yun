/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "standard_csv_reader.h"

LabelManager *StandardCsvReader::loading_label()
{
    LabelManager *label_manager = new LabelManager();
    // === loading Label.csv ===
    spdlog::info("Loading {} ...", LabelSchema::file_name);
    const std::string label_file_path = this->root_dir + "/" + LabelSchema::file_name;
    FileUtils::file_exists(label_file_path, LabelSchema::file_name);
    rapidcsv::Document label_doc(label_file_path);
    // 校验 label 文件列是否缺失
    CsvUtils::check_column_exist(label_doc, LabelSchema::headers, LabelSchema::file_name);

    int label_count = label_doc.GetRowCount();
    for (int u = 0; u < label_count; u++)
    {
        const std::string dim_code = label_doc.GetCell<std::string>(LabelSchema::headers[LabelSchema::CODE], u);
        const std::string dim_name = label_doc.GetCell<std::string>(LabelSchema::headers[LabelSchema::NAME], u);
        label_manager->create_label(dim_code, dim_name);
    }
    label_doc.Clear();
    spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.", 
        LabelSchema::file_name, label_count);

    // === loading LabelValue.csv ===
    spdlog::info("Loading {} ...", LabelValueSchema::file_name);
    const std::string label_value_file_path = this->root_dir + "/" + LabelValueSchema::file_name;
    FileUtils::file_exists(label_value_file_path, LabelValueSchema::file_name);
    rapidcsv::Document label_value_doc(label_value_file_path);
    // 校验 label value 文件列是否缺失
    CsvUtils::check_column_exist(label_value_doc, LabelValueSchema::headers, LabelValueSchema::file_name);

    int label_value_count = label_value_doc.GetRowCount();
    for (int u = 0; u < label_value_count; u++)
    {
        const std::string label_code = label_value_doc.GetCell<std::string>(LabelValueSchema::headers[LabelValueSchema::LABEL_CODE], u);
        const std::string label_value = label_value_doc.GetCell<std::string>(LabelValueSchema::headers[LabelValueSchema::LABEL_VALUE], u);
        Label *label = label_manager->get_label(label_code);
        if (label == nullptr)
        {
            throw std::runtime_error(std::string(LabelValueSchema::file_name) + " label_code: " + label_code + " not found");
        }
        LabelValue *label_value_ptr = label->add_label_value(label_value);
        if (label_value_ptr == nullptr)
        {
            throw std::runtime_error(std::string(LabelValueSchema::file_name) + " label_value: " + label_value + " is repetitive");
        }
    }
    label_value_doc.Clear();
    spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.", 
        LabelValueSchema::file_name, label_value_count);

    // === loading LabelApply.csv ===
    spdlog::info("Loading {} ...", LabelApplySchema::file_name);
    const std::string label_apply_file_path = this->root_dir + "/" + LabelApplySchema::file_name;
    FileUtils::file_exists(label_apply_file_path, LabelApplySchema::file_name);
    rapidcsv::Document label_apply_doc(label_apply_file_path);
    // 校验 label apply 文件列是否缺失
    CsvUtils::check_column_exist(label_apply_doc, LabelApplySchema::headers, LabelApplySchema::file_name);

    int label_apply_row_count = label_apply_doc.GetRowCount();
    for (int u = 0; u < label_apply_row_count; u++)
    {
        const std::string label_code = label_apply_doc.GetCell<std::string>(LabelApplySchema::headers[LabelApplySchema::LABEL_CODE], u);
        const std::string apply_item = label_apply_doc.GetCell<std::string>(LabelApplySchema::headers[LabelApplySchema::APPLY_ITEM], u);
        // 校验 label code 是否存在
        if (label_manager->get_label(label_code) == nullptr)
        {
            throw std::runtime_error(std::string(LabelApplySchema::file_name) + " label code: " + label_code + " not exist");
        }
        // 解析label和实体的映射关系
        if (!LabelSetSupportItem::item_map.count(apply_item))
        {
            throw std::runtime_error(std::string(LabelApplySchema::file_name) + " apply item: " + apply_item + " not support");
        }
        label_manager->labelset_add_label(label_code, apply_item);
    }
    label_apply_doc.Clear();
    spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.", 
        LabelApplySchema::file_name, label_apply_row_count);

    return label_manager;
}