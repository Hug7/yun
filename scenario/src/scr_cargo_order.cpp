/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <format>
#include <string>

#include "c_csv_utils.h"
#include "c_file_utils.h"
#include "c_rapidcsv.h"
#include "c_time_utils.h"
#include "se_schema.h"
#include "standard_csv_reader.h"

CargoOrderManager* StandardCsvReader::loading_cargo_order(LocationManager* location_manager,
                                                          DimensionManager* dimension_manager,
                                                          LabelManager* label_manager) {
  CargoOrderManager* cargo_order_manager =
      new CargoOrderManager(label_manager->order_labelset, dimension_manager);
  // === loading CargoOrder.csv ===
  spdlog::info("Loading {} ...", CargoOrderSchema::file_name);
  const std::string cargo_order_file_path = this->root_dir + "/" + CargoOrderSchema::file_name;
  FileUtils::file_exists(cargo_order_file_path, CargoOrderSchema::file_name);
  rapidcsv::Document cargo_order_doc(cargo_order_file_path);
  CsvUtils::check_column_exist(cargo_order_doc, CargoOrderSchema::headers,
                               CargoOrderSchema::file_name);

  int cargo_order_count = cargo_order_doc.GetRowCount();
  for (int u = 0; u < cargo_order_count; u++) {
    const std::string cargo_order_code =
        cargo_order_doc.GetCell<std::string>(CargoOrderSchema::headers[CargoOrderSchema::CODE], u);
    const std::string cargo_order_name =
        cargo_order_doc.GetCell<std::string>(CargoOrderSchema::headers[CargoOrderSchema::NAME], u);
    const std::string pick_location_code = cargo_order_doc.GetCell<std::string>(
        CargoOrderSchema::headers[CargoOrderSchema::PICK_LOCATION_CODE], u);
    const std::string drop_location_code = cargo_order_doc.GetCell<std::string>(
        CargoOrderSchema::headers[CargoOrderSchema::DROP_LOCATION_CODE], u);
    const std::string earliest_pick_date_time_str = cargo_order_doc.GetCell<std::string>(
        CargoOrderSchema::headers[CargoOrderSchema::EARLIEST_PICK_DATE_TIME], u);
    const std::string latest_pick_date_time_str = cargo_order_doc.GetCell<std::string>(
        CargoOrderSchema::headers[CargoOrderSchema::LATEST_PICK_DATE_TIME], u);
    const std::string earliest_drop_date_time_str = cargo_order_doc.GetCell<std::string>(
        CargoOrderSchema::headers[CargoOrderSchema::EARLIEST_DROP_DATE_TIME], u);
    const std::string latest_drop_date_time_str = cargo_order_doc.GetCell<std::string>(
        CargoOrderSchema::headers[CargoOrderSchema::LATEST_DROP_DATE_TIME], u);

    if (cargo_order_manager->get_cargo_order(cargo_order_code) != nullptr) {
      throw std::runtime_error("Duplicate cargo_order_code " + cargo_order_code + " in" +
                               std::string(CargoOrderSchema::file_name));
    }

    Location* pick_location = location_manager->get_location(pick_location_code);
    if (pick_location == nullptr) {
      throw std::runtime_error("pick location " + pick_location_code + " for order " +
                               cargo_order_code + " not found in" +
                               std::string(CargoOrderSchema::file_name));
    }
    Location* drop_location = location_manager->get_location(drop_location_code);
    if (drop_location == nullptr) {
      throw std::runtime_error("drop location " + pick_location_code + " for order " +
                               cargo_order_code + " not found in" +
                               std::string(CargoOrderSchema::file_name));
    }
    long earliest_pick_time_sec =
        TimeParse::parse_tm_to_sec(earliest_pick_date_time_str, TimeParse::fmt_yyyymmddhhmm_1);
    long latest_pick_time_sec =
        TimeParse::parse_tm_to_sec(latest_pick_date_time_str, TimeParse::fmt_yyyymmddhhmm_1);
    long earliest_drop_time_sec =
        TimeParse::parse_tm_to_sec(earliest_drop_date_time_str, TimeParse::fmt_yyyymmddhhmm_1);
    long latest_drop_time_sec =
        TimeParse::parse_tm_to_sec(latest_drop_date_time_str, TimeParse::fmt_yyyymmddhhmm_1);
    if (earliest_pick_time_sec > latest_pick_time_sec) {
      throw std::runtime_error(std::format(
          "The earliest pick time for Order {} cannot be later than the latest pick time.",
          cargo_order_code));
    }
    if (earliest_drop_time_sec > latest_drop_time_sec) {
      throw std::runtime_error(std::format(
          "The earliest drop time for Order {} cannot be later than the latest drop time.",
          cargo_order_code));
    }
    if (earliest_pick_time_sec > earliest_drop_time_sec) {
      throw std::runtime_error(std::format(
          "The earliest pick time for Order {} cannot be later than the earliest drop time.",
          cargo_order_code));
    }

    CargoOrder* cargo_order = cargo_order_manager->create_cargo_order(
        cargo_order_code, cargo_order_name, pick_location, drop_location);
    cargo_order->set_pick_time_window(new TimeWindow(earliest_pick_time_sec, latest_pick_time_sec));
    cargo_order->set_drop_time_window(new TimeWindow(earliest_drop_time_sec, latest_drop_time_sec));
  }
  cargo_order_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               CargoOrderSchema::file_name, cargo_order_count);

  // === loading CargoSubOrder.csv ===
  const std::string cargo_sub_order_file_path =
      this->root_dir + "/" + CargoSubOrderSchema::file_name;
  FileUtils::file_exists(cargo_sub_order_file_path, CargoSubOrderSchema::file_name);
  rapidcsv::Document cargo_sub_order_doc(cargo_sub_order_file_path);
  CsvUtils::check_column_exist(cargo_sub_order_doc, CargoSubOrderSchema::headers,
                               CargoSubOrderSchema::file_name);

  int cargo_sub_order_count = cargo_sub_order_doc.GetRowCount();
  for (int u = 0; u < cargo_sub_order_count; u++) {
    const std::string cargo_order_code = cargo_sub_order_doc.GetCell<std::string>(
        CargoSubOrderSchema::headers[CargoSubOrderSchema::CARGO_ORDER_CODE], u);
    const std::string cargo_sub_order_code = cargo_sub_order_doc.GetCell<std::string>(
        CargoSubOrderSchema::headers[CargoSubOrderSchema::CARGO_SUB_ORDER_CODE], u);
    const std::string cargo_sub_order_name = cargo_sub_order_doc.GetCell<std::string>(
        CargoSubOrderSchema::headers[CargoSubOrderSchema::CARGO_SUB_ORDER_NAME], u);
    const int quantity = cargo_sub_order_doc.GetCell<int>(
        CargoSubOrderSchema::headers[CargoSubOrderSchema::QUANTITY], u);

    CargoOrder* cargo_order = cargo_order_manager->get_cargo_order(cargo_order_code);
    if (cargo_order == nullptr) {
      throw std::runtime_error("CargoOrder " + cargo_order_code + " not exist in" +
                               std::string(CargoSubOrderSchema::file_name));
    }
    if (cargo_order->get_cargo_sub_order(cargo_sub_order_code) != nullptr) {
      throw std::runtime_error("Duplicate suborder " + cargo_sub_order_code + " for order " +
                               cargo_order_code + " in " +
                               std::string(CargoSubOrderSchema::file_name));
    }

    cargo_order_manager->create_cargo_sub_order(cargo_order_code, cargo_sub_order_code,
                                                cargo_sub_order_name, quantity);
  }
  cargo_sub_order_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               CargoSubOrderSchema::file_name, cargo_sub_order_count);

  // === loading CargoSubOrderDimensionValue.csv ===
  const std::string cargo_sub_order_dimension_value_file_path =
      this->root_dir + "/" + CargoSubOrderDimensionValueSchema::file_name;
  FileUtils::file_exists(cargo_sub_order_dimension_value_file_path,
                         CargoSubOrderDimensionValueSchema::file_name);
  rapidcsv::Document cargo_sub_order_dimension_doc(cargo_sub_order_dimension_value_file_path);
  CsvUtils::check_column_exist(cargo_sub_order_dimension_doc,
                               CargoSubOrderDimensionValueSchema::headers,
                               CargoSubOrderDimensionValueSchema::file_name);

  int cargo_sub_order_dimension_count = cargo_sub_order_dimension_doc.GetRowCount();
  for (int u = 0; u < cargo_sub_order_dimension_count; u++) {
    const std::string cargo_order_code = cargo_sub_order_dimension_doc.GetCell<std::string>(
        CargoSubOrderDimensionValueSchema::headers
            [CargoSubOrderDimensionValueSchema::CARGO_ORDER_CODE],
        u);
    const std::string cargo_sub_order_code = cargo_sub_order_dimension_doc.GetCell<std::string>(
        CargoSubOrderDimensionValueSchema::headers
            [CargoSubOrderDimensionValueSchema::CARGO_SUB_ORDER_CODE],
        u);
    const std::string dimension_code = cargo_sub_order_dimension_doc.GetCell<std::string>(
        CargoSubOrderDimensionValueSchema::headers
            [CargoSubOrderDimensionValueSchema::DIMENSION_CODE],
        u);
    const double dimension_value = cargo_sub_order_dimension_doc.GetCell<double>(
        CargoSubOrderDimensionValueSchema::headers
            [CargoSubOrderDimensionValueSchema::DIMENSION_VALUE],
        u);
    CargoOrder* cargo_order = cargo_order_manager->get_cargo_order(cargo_order_code);
    if (cargo_order == nullptr) {
      throw std::runtime_error("CargoOrder " + cargo_order_code + " not exist in" +
                               std::string(CargoSubOrderSchema::file_name));
    }
    CargoSubOrder* cargo_sub_order = cargo_order->get_cargo_sub_order(cargo_sub_order_code);
    if (cargo_sub_order == nullptr) {
      throw std::runtime_error("CargoSubOrder " + cargo_sub_order_code + " for order " +
                               cargo_order_code + " not exist in" +
                               std::string(CargoSubOrderSchema::file_name));
    }
    Dimension* dimension = dimension_manager->get_dimension(dimension_code);
    if (dimension == nullptr) {
      throw std::runtime_error("Dimension " + dimension_code + " not exist in" +
                               std::string(CargoSubOrderSchema::file_name));
    }
    cargo_sub_order->update_dim_value(dimension, dimension_value);
  }
  cargo_sub_order_dimension_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               CargoSubOrderDimensionValueSchema::file_name, cargo_sub_order_dimension_count);

  // === loading CargoSubOrderLabelValue.csv ===
  spdlog::info("Loading {} ...", CargoSubOrderLabelValueSchema::file_name);
  const std::string cargo_sub_order_label_value_file_path =
      this->root_dir + "/" + CargoSubOrderLabelValueSchema::file_name;
  FileUtils::file_exists(cargo_sub_order_label_value_file_path,
                         CargoSubOrderLabelValueSchema::file_name);
  rapidcsv::Document cargo_sub_order_label_doc(cargo_sub_order_label_value_file_path);
  CsvUtils::check_column_exist(cargo_sub_order_label_doc, CargoSubOrderLabelValueSchema::headers,
                               CargoSubOrderLabelValueSchema::file_name);

  int cargo_sub_order_label_count = cargo_sub_order_label_doc.GetRowCount();
  for (int u = 0; u < cargo_sub_order_label_count; u++) {
    const std::string cargo_order_code = cargo_sub_order_label_doc.GetCell<std::string>(
        CargoSubOrderLabelValueSchema::headers[CargoSubOrderLabelValueSchema::CARGO_ORDER_CODE], u);
    const std::string cargo_sub_order_code = cargo_sub_order_label_doc.GetCell<std::string>(
        CargoSubOrderLabelValueSchema::headers[CargoSubOrderLabelValueSchema::CARGO_SUB_ORDER_CODE],
        u);
    const std::string label_code = cargo_sub_order_label_doc.GetCell<std::string>(
        CargoSubOrderLabelValueSchema::headers[CargoSubOrderLabelValueSchema::LABEL_CODE], u);
    const std::string label_value_str = cargo_sub_order_label_doc.GetCell<std::string>(
        CargoSubOrderLabelValueSchema::headers[CargoSubOrderLabelValueSchema::LABEL_VALUE], u);
    CargoOrder* cargo_order = cargo_order_manager->get_cargo_order(cargo_order_code);
    if (cargo_order == nullptr) {
      throw std::runtime_error("CargoOrder " + cargo_order_code + " not exist in" +
                               std::string(CargoSubOrderSchema::file_name));
    }
    CargoSubOrder* cargo_sub_order = cargo_order->get_cargo_sub_order(cargo_sub_order_code);
    if (cargo_sub_order == nullptr) {
      throw std::runtime_error("CargoSubOrder " + cargo_sub_order_code + " for order " +
                               cargo_order_code + " not exist in" +
                               std::string(CargoSubOrderSchema::file_name));
    }
    Label* label = label_manager->get_label(label_code);
    if (label == nullptr) {
      throw std::runtime_error("Label " + label_code + " not exist for labels in" +
                               std::string(CargoSubOrderSchema::file_name));
    }
    LabelValue* label_value = label->get_label_value(label_value_str);
    if (label_value == nullptr) {
      throw std::runtime_error("LabelValue " + label_value_str + " for label " + label_code +
                               " not exist in" + std::string(CargoSubOrderSchema::file_name));
    }
    int label_ind4labelset = cargo_order_manager->labelset->get_label_ind(label_code);
    if (label_ind4labelset == -1) {
      throw std::runtime_error("Label " + label_code + " not exist in cargo labelset");
    }
    cargo_sub_order->update_labelset_value(label_ind4labelset, label_value);
  }
  cargo_sub_order_label_doc.Clear();
  spdlog::info("Loading {} is complete. A total of {} pieces of data have been obtained.",
               CargoSubOrderLabelValueSchema::file_name, cargo_sub_order_label_count);

  return cargo_order_manager;
}
