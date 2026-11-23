/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/null_sink.h>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "bdm_available_vehicle.h"
#include "bdm_cargo_order.h"
#include "bdm_carrier.h"
#include "bdm_common.h"
#include "bdm_dimension.h"
#include "bdm_label.h"
#include "bdm_location.h"
#include "bdm_vehicle.h"
#include "bdm_vehicle_model.h"
#include "c_yaml_utils.h"
#include "hc_constraint.h"
#include "hc_location.h"
#include "hc_manager.h"
#include "hc_vehicle.h"
#include "pdm_context.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "pdm_parameter.h"
#include "pdm_resource.h"
#include "pdm_segment.h"
#include "pdm_time_window_pattern.h"
#include "prob_pattern.h"
#include "se_scenario.h"

namespace {

// 预检测试只需要 scenario 中与 load/node/resource 相关的最小基建，
// 与 hc_label_value_test 的 MakeContext 保持同一套装配方式。
class HcPrecheckTest : public ::testing::Test {
 protected:
  void SetUp() override {
    this->scenario = new Scenario{};
    this->dim_manager = new DimensionManager();
    this->label_manager = new LabelManager();
    this->location_manager =
        new LocationManager(this->label_manager->location_labelset, this->dim_manager);
    this->carrier_manager = new CarrierManager(this->label_manager->carrier_labelset);
    this->cargo_order_manager =
        new CargoOrderManager(this->label_manager->order_labelset, this->dim_manager);
    this->scenario->set_dim_manager(this->dim_manager);
    this->scenario->set_label_manager(this->label_manager);
    this->scenario->set_location_manager(this->location_manager);
    this->scenario->set_carrier_manager(this->carrier_manager);
    this->scenario->set_cargo_order_manager(this->cargo_order_manager);
    this->dim_manager->create_dimension("weight", "weight", 0);

    // 订单标签集合加一个 4 取值标签，供标签值预检测试使用
    auto* order_label =
        new Label("temperature", "temperature",
                  static_cast<int>(this->label_manager->order_labelset->labels.size()));
    for (const char* value : {"cold", "ambient", "dangerous", "fragile"}) {
      order_label->add_label_value(value);
    }
    this->label_manager->order_labelset->add_label(order_label);

    auto logger = std::make_shared<spdlog::logger>("hc_precheck_test",
                                                   std::make_shared<spdlog::sinks::null_sink_mt>());
    this->parameter = new Parameter(logger);
    this->parameter->plan_datetime_range = new PlanDatetimeRange();
    this->context = new LoadContext(this->scenario, this->parameter);
    this->carrier = this->carrier_manager->create_carrier("C0", "carrier");
  }

  VehicleModel* MakeVehicleModel(const std::string& code, int ind, std::int64_t capacity) {
    std::vector<std::int64_t> dim_vals = this->dim_manager->empty_dim_values();
    dim_vals[0] = capacity;
    return new VehicleModel(code, code, ind, nullptr, nullptr, dim_vals, nullptr, nullptr);
  }

  Vehicle* MakeVehicle(VehicleModel* vehicle_model) {
    return this->carrier_manager->create_vehicle(this->carrier->code, vehicle_model, 1, nullptr,
                                                 nullptr);
  }

  Bitset BitsFor(const std::vector<Vehicle*>& vehicles) {
    Bitset bitset = this->carrier_manager->empty_vehicle_bitset();
    for (Vehicle* vehicle : vehicles) {
      bitset.set(vehicle->ind);
    }
    return bitset;
  }

  Location* MakeLocation(const std::string& code) {
    return this->location_manager->create_location(code, code, 0.0, 0.0);
  }

  // 站点可用车型 = 当前已创建的全部车型
  Location* MakeFullLocation(const std::string& code) {
    Location* location = this->MakeLocation(code);
    location->set_available_vehicle(this->carrier_manager->full_available_vehicle());
    return location;
  }

  Order* MakeOrder(int ind, Location* pick_loc, Location* drop_loc, std::int64_t weight,
                   const std::vector<int>& label_values = {}) {
    auto* cargo_order = this->cargo_order_manager->create_cargo_order("CO" + std::to_string(ind),
                                                                      "co", pick_loc, drop_loc);
    const TimeWindow tw = this->parameter->plan_datetime_range->create_default_time_window();
    cargo_order->set_pick_time_window(tw);
    cargo_order->set_drop_time_window(tw);
    std::vector<std::int64_t> dim_vals = this->dim_manager->empty_dim_values();
    dim_vals[0] = weight;
    auto labelset_value = this->label_manager->order_labelset->empty_labelset_value();
    auto labelset_value_bitset = this->label_manager->order_labelset->empty_labelset_value_bitset();
    for (const int value_ind : label_values) {
      labelset_value_bitset->set(0, value_ind);
    }
    cargo_order->create_cargo_sub_order("SUB" + std::to_string(ind), "sub", dim_vals,
                                        labelset_value, std::move(labelset_value_bitset), 1);
    std::vector<CargoOrder*> cargo_orders = {cargo_order};
    return OrderFactory::creat_order(cargo_orders, ind, this->parameter->plan_datetime_range,
                                     this->scenario, &this->tw_pattern_registry);
  }

  LoadSPMD* MakeLoad(const std::vector<Order*>& orders, const Vehicle* vehicle) {
    auto* load = this->pattern.create_load(this->context);
    for (const Order* order : orders) {
      this->pattern.insert_last_delivery(load, order);
    }
    load->vehicle = vehicle;
    return load;
  }

  VehicleResource MakeVehicleResource(const std::vector<Vehicle*>& vehicles) {
    VehicleUsage::VecUPtr usages;
    for (Vehicle* vehicle : vehicles) {
      usages.push_back(std::make_unique<VehicleUsage>(vehicle));
    }
    return VehicleResource(std::move(usages));
  }

  Scenario* scenario{nullptr};
  DimensionManager* dim_manager{nullptr};
  LabelManager* label_manager{nullptr};
  LocationManager* location_manager{nullptr};
  CarrierManager* carrier_manager{nullptr};
  CargoOrderManager* cargo_order_manager{nullptr};
  Parameter* parameter{nullptr};
  LoadContext* context{nullptr};
  Carrier* carrier{nullptr};
  PatternSPMD pattern;
  TimeWindowPatternRegistry tw_pattern_registry;
};

TEST_F(HcPrecheckTest, CapacityPrunesWhenNoUsableVehicleFits) {
  Vehicle* small = this->MakeVehicle(this->MakeVehicleModel("small", 0, 5));
  Vehicle* big = this->MakeVehicle(this->MakeVehicleModel("big", 1, 10));
  Location* pick = this->MakeFullLocation("L0");
  Location* drop1 = this->MakeFullLocation("L1");
  Location* drop2 = this->MakeFullLocation("L2");

  LoadSPMD* load = this->MakeLoad({this->MakeOrder(0, pick, drop1, 3)}, small);
  Order* segment_order = this->MakeOrder(1, pick, drop2, 3);
  const Segment segment({segment_order});
  const HcVehicleCapacity capacity;

  // 当前车型装不下, 唯一可用车型也装不下: 6 > 5
  VehicleResource only_small = this->MakeVehicleResource({small});
  EXPECT_EQ(capacity.precheck({load, &segment, &only_small}), HcPrecheckVerdict::kInfeasible);

  // 可切换到更大的车型时不剪枝
  VehicleResource both = this->MakeVehicleResource({small, big});
  EXPECT_EQ(capacity.precheck({load, &segment, &both}), HcPrecheckVerdict::kPass);

  delete load;
  load = this->MakeLoad({this->MakeOrder(2, pick, drop1, 3)}, big);
  EXPECT_EQ(capacity.precheck({load, &segment, &only_small}), HcPrecheckVerdict::kPass);
  delete load;

  // 预检用的峰值推算必须与真实插入后的峰值一致, 否则剪枝会误杀可行插入
  load = this->MakeLoad({this->MakeOrder(3, pick, drop1, 3)}, small);
  const Segment check({this->MakeOrder(4, pick, drop2, 3)});
  const auto inserted =
      this->pattern.insert_segment_at(load, &check, InsertSlot(ActivityType::DROP, 0));
  ASSERT_FALSE(inserted.empty());
  EXPECT_EQ(load->get_peak_load_dims()[0], 6);
  delete load;
}

TEST_F(HcPrecheckTest, NodeCountBounds) {
  Vehicle* vehicle = this->MakeVehicle(this->MakeVehicleModel("small", 0, 100));
  Location* pick = this->MakeFullLocation("L0");
  Location* drop1 = this->MakeFullLocation("L1");
  Location* drop2 = this->MakeFullLocation("L2");
  Location* drop3 = this->MakeFullLocation("L3");
  Location* drop4 = this->MakeFullLocation("L4");

  LoadSPMD* load = this->MakeLoad(
      {this->MakeOrder(0, pick, drop1, 1), this->MakeOrder(1, pick, drop2, 1)}, vehicle);
  ASSERT_EQ(load->get_drop_node_count(), 2);
  ASSERT_EQ(load->get_pick_node_count(), 1);

  // 段内两个订单落在同一新站点: 至多新增 1 个节点, 且必定新增 1 个
  const Segment new_drop_segment(
      {this->MakeOrder(2, pick, drop3, 1), this->MakeOrder(3, pick, drop3, 1)});
  const HcMaxDropNodeCount max_drop_two(2);
  const HcMaxDropNodeCount max_drop_three(3);
  VehicleResource resource = this->MakeVehicleResource({vehicle});
  EXPECT_EQ(max_drop_two.precheck({load, &new_drop_segment, &resource}),
            HcPrecheckVerdict::kInfeasible);
  EXPECT_EQ(max_drop_three.precheck({load, &new_drop_segment, &resource}),
            HcPrecheckVerdict::kPass);

  // 段内站点已在车次链上: 可能合并也可能新增, 不足以判定
  const Segment existing_drop_segment({this->MakeOrder(4, pick, drop1, 1)});
  EXPECT_EQ(max_drop_two.precheck({load, &existing_drop_segment, &resource}),
            HcPrecheckVerdict::kUnknown);

  // 提货侧: SPMD 下提货节点数不增加, 只有上限为 0 时才能确定违背
  const Segment pick_segment({this->MakeOrder(5, pick, drop4, 1)});
  const HcMaxPickNodeCount max_pick_one(1);
  const HcMaxPickNodeCount max_pick_zero(0);
  EXPECT_EQ(max_pick_one.precheck({load, &pick_segment, &resource}), HcPrecheckVerdict::kUnknown);
  EXPECT_EQ(max_pick_zero.precheck({load, &pick_segment, &resource}),
            HcPrecheckVerdict::kInfeasible);
  delete load;
}

TEST_F(HcPrecheckTest, AvailableVehiclePrunesWhenIntersectionEmpty) {
  Vehicle* vehicle0 = this->MakeVehicle(this->MakeVehicleModel("small", 0, 100));
  Vehicle* vehicle1 = this->MakeVehicle(this->MakeVehicleModel("big", 1, 100));
  Location* pick = this->MakeFullLocation("L0");
  Location* drop = this->MakeFullLocation("L1");
  Location* only_first = this->MakeLocation("L2");
  only_first->set_available_vehicle(new AvailableVehicle({vehicle0}, this->BitsFor({vehicle0})));
  Location* unreachable = this->MakeLocation("L3");
  unreachable->set_available_vehicle(new AvailableVehicle({}, this->BitsFor({})));

  LoadSPMD* load = this->MakeLoad({this->MakeOrder(0, pick, drop, 1)}, vehicle0);
  VehicleResource resource = this->MakeVehicleResource({vehicle0, vehicle1});
  const HcAvailableVehicle available_vehicle;

  const Segment partial({this->MakeOrder(1, pick, only_first, 1)});
  EXPECT_EQ(available_vehicle.precheck({load, &partial, &resource}), HcPrecheckVerdict::kPass);

  const Segment none({this->MakeOrder(2, pick, unreachable, 1)});
  EXPECT_EQ(available_vehicle.precheck({load, &none, &resource}), HcPrecheckVerdict::kInfeasible);
  delete load;
}

TEST_F(HcPrecheckTest, LabelValuePrechecksUsePostInsertionUnion) {
  Vehicle* vehicle = this->MakeVehicle(this->MakeVehicleModel("small", 0, 100));
  Location* pick = this->MakeFullLocation("L0");
  Location* drop = this->MakeFullLocation("L1");
  Location* drop2 = this->MakeFullLocation("L2");

  const LabelValueScope scope{this->label_manager->order_labelset, 0, LabelValueEntity::Order,
                              LabelValueSide::Both};
  const HcLabelValueExclusive exclusive(scope, {{0, 1}});
  const HcLabelValueMaxCount max_count_two(scope, 2);
  VehicleResource resource = this->MakeVehicleResource({vehicle});

  // 基线订单带 cold(0), 待插订单带 ambient(1)
  LoadSPMD* load = this->MakeLoad({this->MakeOrder(0, pick, drop, 1, {0})}, vehicle);
  const Segment conflict({this->MakeOrder(1, pick, drop2, 1, {1})});
  EXPECT_EQ(exclusive.precheck({load, &conflict, &resource}), HcPrecheckVerdict::kInfeasible);

  // 待插订单带 dangerous(2): 不互斥, 种类数 0/2 共 2 种不超过 2
  const Segment compatible({this->MakeOrder(2, pick, drop2, 1, {2})});
  EXPECT_EQ(exclusive.precheck({load, &compatible, &resource}), HcPrecheckVerdict::kPass);
  EXPECT_EQ(max_count_two.precheck({load, &compatible, &resource}), HcPrecheckVerdict::kPass);

  const HcLabelValueMaxCount max_count_one(scope, 1);
  EXPECT_EQ(max_count_one.precheck({load, &compatible, &resource}), HcPrecheckVerdict::kInfeasible);
  delete load;
}

TEST_F(HcPrecheckTest, ManagerHonoursSwitchAndSkipsUntunableCases) {
  Vehicle* vehicle = this->MakeVehicle(this->MakeVehicleModel("small", 0, 5));
  Location* pick = this->MakeFullLocation("L0");
  Location* drop = this->MakeFullLocation("L1");
  Location* drop2 = this->MakeFullLocation("L2");
  LoadSPMD* load = this->MakeLoad({this->MakeOrder(0, pick, drop, 3)}, vehicle);
  const Segment segment({this->MakeOrder(1, pick, drop2, 3)});
  VehicleResource resource = this->MakeVehicleResource({vehicle});

  // 默认关闭预检: 不做任何推断
  ASSERT_FALSE(this->parameter->hard_constr_precheck);
  HardConstraintManager disabled_manager(this->scenario, this->parameter);
  EXPECT_EQ(disabled_manager.precheck(load, &segment, resource), HcPrecheckVerdict::kUnknown);

  // 开关打开后, 容量约束确定违背, manager 短路返回 kInfeasible
  HardConstraintManager enabled_manager(this->scenario, this->parameter);
  enabled_manager.precheck_enabled = true;
  EXPECT_EQ(enabled_manager.precheck(load, &segment, resource), HcPrecheckVerdict::kInfeasible);

  // 空片段不参与预检
  const std::vector<const Order*> no_orders;
  const Segment empty_segment(no_orders);
  EXPECT_EQ(enabled_manager.precheck(load, &empty_segment, resource), HcPrecheckVerdict::kUnknown);
  delete load;
}

TEST(HcPrecheckParameterTest, ParsesPrecheckSwitch) {
  auto logger = std::make_shared<spdlog::logger>("hc_precheck_parameter_test",
                                                 std::make_shared<spdlog::sinks::null_sink_mt>());
  Parameter parameter(logger);
  EXPECT_FALSE(parameter.hard_constr_precheck);

  YAML::Node root;
  parameter.parse_hard_constr_precheck(YamlNode(root));
  EXPECT_FALSE(parameter.hard_constr_precheck);

  root["hard_constr_precheck"] = true;
  parameter.parse_hard_constr_precheck(YamlNode(root));
  EXPECT_TRUE(parameter.hard_constr_precheck);

  root["hard_constr_precheck"] = "not-a-bool";
  EXPECT_THROW(parameter.parse_hard_constr_precheck(YamlNode(root)), std::runtime_error);
}

}  // namespace
