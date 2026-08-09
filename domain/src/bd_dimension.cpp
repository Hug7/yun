/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bd_dimension.h"

// ====== implement of DimensionManager ======
DimensionManager::~DimensionManager()
{
    for (auto &d : this->dimensions)
    {
        delete d;
    }
    this->dimensions.clear();
    this->dimension_map.clear();
}

Dimension *DimensionManager::create_dimension(const std::string &code, const std::string &name, const int precision)
{
    if (this->dimension_map.count(code))
    {
        throw new std::runtime_error("Dimension with code " + code + " already exists.");
    }
    if (precision < 0 || precision > 4) {
        throw new std::runtime_error("Precision = " + std::to_string(precision) + " , but must be between 0 and 4.");
    }

    Dimension *dimension = new Dimension(code, name, this->generate_index->next(), precision);
    this->dimensions.push_back(dimension);
    this->dimension_map[code] = dimension;
    ++this->len;
    return dimension;
}

Dimension *DimensionManager::get_dimension(std::string code)
{
    auto iter = this->dimension_map.find(code);
    if (iter != this->dimension_map.end())
    {
        return iter->second;
    }
    else
    {
        return nullptr;
    }
}


std::vector<long> DimensionManager::empty_dim_values()
{
    return std::vector<long>(this->len, 0);
}