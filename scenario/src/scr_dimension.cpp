/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "standard_csv_reader.h"

DimensionManager *StandardCsvReader::loading_dimension()
{
    spdlog::info("Loading {} ...", DimensionSchema::file_name);
    const std::string dimension_file_path = this->root_dir + "/" + DimensionSchema::file_name;
    FileUtils::file_exists(dimension_file_path, DimensionSchema::file_name);
    rapidcsv::Document dim_doc(dimension_file_path);
    CsvUtils::check_column_exist(dim_doc, DimensionSchema::headers, DimensionSchema::file_name);

    DimensionManager *dimension_manager = new DimensionManager();
    int row_count = dim_doc.GetRowCount();
    for (int u = 0; u < row_count; u++)
    {
        const std::string dimension_code = dim_doc.GetCell<std::string>(DimensionSchema::headers[DimensionSchema::CODE], u);
        const std::string dimension_name = dim_doc.GetCell<std::string>(DimensionSchema::headers[DimensionSchema::NAME], u);
        const int percision = dim_doc.GetCell<int>(DimensionSchema::headers[DimensionSchema::PRECISION], u);

        dimension_manager->create_dimension(dimension_code, dimension_name, percision);
    }
    dim_doc.Clear();
    spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.", DimensionSchema::file_name, row_count);

    return dimension_manager;
}