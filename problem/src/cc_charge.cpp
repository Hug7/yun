/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cc_charge.h"

#include <vector>

#include "pdm_infeasible.h"
#include "pdm_load.h"
#include "pdm_order.h"

namespace {

/**
 * @brief 线程本地缓冲，避免每次 eval 分配
 */
thread_local std::vector<const Order*> tls_orders;
thread_local std::vector<const Location*> tls_stops;
thread_local std::vector<const Location*> tls_order_pick_locs;
thread_local std::vector<const Location*> tls_order_drop_locs;

/**
 * @brief Load 的懒加载计费视图
 * @details
 * ChargeManager 只在规则真正需要某项数据时调用对应 getter；getter 第一次调用时
 * 才遍历 Load 的节点链，并把结果缓存在本对象中。
 */
class LoadChargeView final : public ChargeLoadView {
 public:
  explicit LoadChargeView(Load* load) : load(load) {}

  [[nodiscard]] int vehicle_ind() const override {
    return load->vehicle != nullptr ? load->vehicle->ind : -1;
  }

  [[nodiscard]] long distance_m() const override { return load->get_total_travel_dist(); }

  [[nodiscard]] int pick_node_count() const override { return load->get_pick_node_count(); }

  [[nodiscard]] int drop_node_count() const override { return load->get_drop_node_count(); }

  [[nodiscard]] const long* dim_vals() const override { return load->get_total_load_dims().data(); }

  [[nodiscard]] const Location* const* stops() const override {
    ensure_stops();
    return tls_stops.data();
  }

  [[nodiscard]] int stop_count() const override {
    ensure_stops();
    return static_cast<int>(tls_stops.size());
  }

  [[nodiscard]] const Location* const* order_pick_locs() const override {
    ensure_orders();
    return tls_order_pick_locs.data();
  }

  [[nodiscard]] const Location* const* order_drop_locs() const override {
    ensure_orders();
    return tls_order_drop_locs.data();
  }

  [[nodiscard]] int order_count() const override {
    ensure_orders();
    return static_cast<int>(tls_order_pick_locs.size());
  }

 private:
  void ensure_stops() const {
    if (this->stops_loaded_) {
      return;
    }
    this->stops_loaded_ = true;
    tls_stops.clear();
    this->load->fill_locations(tls_stops);
  }

  void ensure_orders() const {
    if (this->orders_loaded_) {
      return;
    }
    this->orders_loaded_ = true;
    tls_orders.clear();
    tls_order_pick_locs.clear();
    tls_order_drop_locs.clear();
    int pre_ind = -1;
    this->load->fill_orders(tls_orders);
    for (const auto& tls_order : tls_orders) {
      if (pre_ind >= 0) {
        if (tls_order_pick_locs[pre_ind] == tls_order->pick_loc &&
            tls_order_drop_locs[pre_ind] == tls_order->drop_loc) {
          continue;
        }
      }
      ++pre_ind;
      tls_order_pick_locs.push_back(tls_order->pick_loc);
      tls_order_drop_locs.push_back(tls_order->drop_loc);
    }
  }

  Load* load;
  mutable bool stops_loaded_{false};
  mutable bool orders_loaded_{false};
};

}  // namespace

// ====== implement of CcCharge ======
CostConstrScore::UPtr CcCharge::eval(Load* load) {
  if (this->charge_manager == nullptr || !this->charge_manager->enabled || load->is_empty() ||
      load->vehicle == nullptr) {
    return std::make_unique<CostConstrScore>(this->code, 1, 0.0, this->seq_type,
                                             this->vehicle_type);
  }

  const LoadChargeView view(load);
  const ChargeResult result = this->charge_manager->evaluate(view);
  if (!result.matched) {
    return std::make_unique<CostConstrScore>(this->code, false, 1, 0.0, this->seq_type,
                                             this->vehicle_type,
                                             InfeasibleReasonCollection::NO_CHARGE_RATE);
  }

  const double value =
      static_cast<double>(result.amount) / static_cast<double>(ChargeParameter::MONEY_SCALE);
  return std::make_unique<CostConstrScore>(this->code, 1, value, this->seq_type,
                                           this->vehicle_type);
}
