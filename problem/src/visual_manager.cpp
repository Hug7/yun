/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visual_manager.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <format>
#include <ranges>
#include <string>
#include <vector>

#include "c_rapidcsv.h"
#include "visual_schema.h"

// ====== implement of Load VisualManager ======
VisualManager::VisualManager(const std::string& output_dir,
                             const std::shared_ptr<spdlog::logger>& logger)
    : output_dir(output_dir), logger(logger) {
  if (!std::filesystem::exists(this->output_dir)) {
    try {
      std::filesystem::create_directories(this->output_dir);
    } catch (const std::exception& e) {
      this->logger->error(std::format("创建文件夹 {} 失败!", this->output_dir));
      this->logger->error(e.what());
    }
  }
}

void VisualManager::infeasible_cargo_order_to_csv(
    InfeasibleCargoOrder::VecUPtr& infeasible_cargo_orders) {
  if (infeasible_cargo_orders.empty()) {
    return;
  }
  // 文件句柄
  rapidcsv::Document doc("", rapidcsv::LabelParams(0, -1));
  // 列头字段集合
  const std::vector<PrecheckInfeasibleCargoOrderSchema::Col> fields = {
      PrecheckInfeasibleCargoOrderSchema::CARGO_ORDER_CODE,
      PrecheckInfeasibleCargoOrderSchema::CARRIER_CODE,
      PrecheckInfeasibleCargoOrderSchema::VEHICLE_MODEL_CODE,
      PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_CODE,
      PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_CONSTRAINT_CODE,
      PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_MESSAGE_CN,
      PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_MESSAGE_EN,
  };
  //
  for (const auto field : fields) {
    doc.SetColumnName(field, PrecheckInfeasibleCargoOrderSchema::headers[field]);
  }

  int cur_row = -1;

  for (const auto& infeasible_cargo_order : infeasible_cargo_orders) {
    for (const auto cargo_order : infeasible_cargo_order->cargo_orders) {
      // 记录-通用的不可解原因
      for (const auto& reason :
           infeasible_cargo_order->common_infeasible_reasons | std::views::values) {
        ++cur_row;
        // 订单号
        doc.SetCell<std::string>(PrecheckInfeasibleCargoOrderSchema::CARGO_ORDER_CODE, cur_row,
                                 cargo_order->code);
        // 不可解原因code
        doc.SetCell<std::string>(PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_CODE,
                                 cur_row, reason.code);
        // 不可解原因对应的约束code
        doc.SetCell<std::string>(
            PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_CONSTRAINT_CODE, cur_row,
            reason.constr_code);
        // 不可解原因-中文
        doc.SetCell<std::string>(PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_MESSAGE_CN,
                                 cur_row, reason.msg_cn);
        // 不可解原因-英文
        doc.SetCell<std::string>(PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_MESSAGE_EN,
                                 cur_row, reason.msg_en);
      }
      // 记录-车辆维度不可解原因
      for (const auto& pair : infeasible_cargo_order->vehicle_infeasible_reasons) {
        for (const auto& reason : pair.second | std::views::values) {
          ++cur_row;
          // 订单号
          doc.SetCell<std::string>(PrecheckInfeasibleCargoOrderSchema::CARGO_ORDER_CODE, cur_row,
                                   cargo_order->code);
          // 承运商
          doc.SetCell<std::string>(PrecheckInfeasibleCargoOrderSchema::CARRIER_CODE, cur_row,
                                   pair.first->carrier->code);
          // 车型
          doc.SetCell<std::string>(PrecheckInfeasibleCargoOrderSchema::VEHICLE_MODEL_CODE, cur_row,
                                   pair.first->vehicle_model->code);
          // 不可解原因code
          doc.SetCell<std::string>(PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_CODE,
                                   cur_row, reason.code);
          // 不可解原因对应的约束code
          doc.SetCell<std::string>(
              PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_CONSTRAINT_CODE, cur_row,
              reason.constr_code);
          // 不可解原因-中文
          doc.SetCell<std::string>(PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_MESSAGE_CN,
                                   cur_row, reason.msg_cn);
          // 不可解原因-英文
          doc.SetCell<std::string>(PrecheckInfeasibleCargoOrderSchema::INFEASIBLE_REASON_MESSAGE_EN,
                                   cur_row, reason.msg_en);
        }  // end for vehicle_infeasible_reasons->val
      }  // end for vehicle_infeasible_reasons
    }  // end for cargo orders
  }  // end for infeasible_cargo_orders

  // 保存文件
  doc.Save(this->output_dir + "/" + PrecheckInfeasibleCargoOrderSchema::file_name);
}
