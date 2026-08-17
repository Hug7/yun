/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string>

#include "c_csv_utils.h"
#include "c_file_utils.h"
#include "c_rapidcsv.h"
#include "se_schema.h"
#include "standard_csv_reader.h"

Parameter* StandardCsvReader::loading_parameter() {
  spdlog::info("Loading {} ...", ParameterSchema::file_name);
  const std::string parameter_file_path = this->root_dir + "/" + ParameterSchema::file_name;
  bool file_exist_flag =
      FileUtils::file_exists(parameter_file_path, ParameterSchema::file_name, false);
  if (!file_exist_flag) {
    return new Parameter();
  }
  rapidcsv::Document parameter_doc(parameter_file_path);
  CsvUtils::check_column_exist(parameter_doc, ParameterSchema::headers, ParameterSchema::file_name);

  Parameter* parameter = new Parameter();
  int row_count = parameter_doc.GetRowCount();
  for (int u = 0; u < row_count; u++) {
    const std::string parameter_code =
        parameter_doc.GetCell<std::string>(ParameterSchema::headers[ParameterSchema::CODE], u);
    const std::string parameter_value =
        parameter_doc.GetCell<std::string>(ParameterSchema::headers[ParameterSchema::VALUE], u);
    // todo parse value of parameter
  }
  parameter_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               ParameterSchema::file_name, row_count);

  return parameter;
}
