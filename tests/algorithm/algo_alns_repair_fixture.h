/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/logger.h>
#include <spdlog/sinks/null_sink.h>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "bdm_available_vehicle.h"
#include "bdm_cargo_order.h"
#include "bdm_carrier.h"
#include "bdm_dimension.h"
#include "bdm_dist_matrix.h"
#include "bdm_label.h"
#include "bdm_location.h"
#include "bdm_vehicle.h"
#include "bdm_vehicle_model.h"
#include "c_yaml_utils.h"
#include "pdm_load.h"
#include "pdm_order.h"
#include "pdm_parameter.h"
#include "pdm_parameter_constraint.h"
#include "pdm_resource.h"
#include "pdm_segment.h"
#include "pdm_solution.h"
#include "prob_load_editor.h"
#include "prob_problem.h"
#include "sc_constraint_code.h"
#include "se_scenario.h"

/**
 * @brief ALNS repair 的 Problem 层内存夹具
 * @details 只依赖 Problem 层(scenario/problem), 不依赖 algorithm 层头文件, 因此基线版
 * golden probe 也可以直接包含它。
 *
 * 确定性数据(全部在构造函数里写死):
 * - 维度: 1 个 "weight"(int, ind 0)
 * - 站点: LocationManager 自动创建的 default(ind 0) + L1..L6(ind 1..6), 共 7 个;
 *   default 与 L1..L6 的 available_vehicle 都指向同一个全车辆 AvailableVehicle(3 辆车)。
 * - 距离矩阵: DistMatrixCode "DM", matrix_len 7,
 *   dist(i,j) = (i==j) ? 0 : i*10+j, time(i,j) = (i==j) ? 0 : i+2*j
 *   (非零、非对称, 时间量级 <= 12s, 远小于计划时间窗)。
 * - 车型: 1 个 "VM", 其能力 weight=100, dist_matrix_code/matrix 指向上面的 "DM"。
 * - 承运商: "C0"; 车辆 3 辆(count=1):
 *     V0: orig L1 -> dest L2
 *     V1: orig L3 -> dest L4
 *     V2: orig L5 -> dest L6
 * - 参数: pick_drop_pattern=SPMD, load_unload_policy=FILO, 一条 SoftConstrCode::Distance
 *   软约束(dist_factor=1.0), 因此 load 的 obj_val == 1.0 * total_travel_dist(严格正数)。
 * - 订单 5 个(weight 见下), 全部 pick_loc=L5(SPMD 下同一 load 的 pick 站点必须一致):
 *     O0: pick L5 -> drop L2  weight 10
 *     O1: pick L5 -> drop L4  weight 10
 *     O2: pick L5 -> drop L6  weight 10
 *     O3: pick L5 -> drop L3  weight 20
 *     O4: pick L5 -> drop L1  weight 30
 * - make_solution(): 先用 load_editor()->build() 逐个构造 O0、O1 两个各 1 单的初始车次
 *   (build 自己选车型, 依次落到 V0、V1, 因为拿到的 vehicle_resource 会随 add_load 占用而变化;
 *   结果: initial load0 = V0 (obj 67), initial load1 = V1 (obj 89), 剩 V2 空闲),
 *   再把 O2、O3、O4 作为未指派订单加入, 因此 solution->unassigned_orders.size()==3,
 *   SegmentBuilder::call(sol.get()).size()==3。
 *
 * 生命周期: make_solution() 返回的 Solution 以及其中的 Load 引用本夹具持有的 Order/场景数据,
 * 必须在本夹具析构之前销毁(测试里让 Solution 的声明晚于夹具即可)。
 */
class AlnsRepairFixture {
 public:
  /** @brief 受管站点数量(L1..L6), 不含 default 站点 */
  static constexpr int kManagedLocationCount = 6;
  /** @brief 车辆数量 */
  static constexpr int kVehicleCount = 3;
  /** @brief 订单总数 */
  static constexpr int kOrderCount = 5;
  /** @brief 初始(已指派)车次数量, 每个车次 1 单 */
  static constexpr int kAssignedOrderCount = 2;
  /** @brief 未指派订单数量(即 SegmentBuilder 产出的 segment 数量) */
  static constexpr int kUnassignedOrderCount = kOrderCount - kAssignedOrderCount;
  /** @brief 车型能力(weight) */
  static constexpr std::int64_t kVehicleCapacity = 100;
  /** @brief 距离软约束因子 */
  static constexpr double kDistFactor = 1.0;

  AlnsRepairFixture();

  ~AlnsRepairFixture();

  AlnsRepairFixture(const AlnsRepairFixture&) = delete;

  AlnsRepairFixture& operator=(const AlnsRepairFixture&) = delete;

  // ===== lead 约定的测试契约 =====

  /** @brief 车次编辑器 */
  [[nodiscard]] LoadEditor* load_editor() const { return this->load_editor_; }

  /** @brief load context */
  [[nodiscard]] LoadContext* load_context() const { return this->problem_->load_context; }

  /**
   * @brief 每次调用返回一份全新的 Solution
   * @details 2 个已指派车次(每个 1 单, 通过 load_editor()->build(orders, *sol->vehicle_resource)
   * 构造) + 3 个未指派订单。调用方持有返回的 Solution。
   */
  [[nodiscard]] std::unique_ptr<Solution> make_solution() const;

  /** @brief 固定返回 3(SegmentBuilder::call(sol.get()) 产出的 segment 数量) */
  [[nodiscard]] static int unassigned_order_count() { return kUnassignedOrderCount; }

  // ===== 附加只读访问器(便于 probe / 调试, 不属于 lead 约定) =====

  [[nodiscard]] const Scenario* scenario() const { return this->scenario_; }

  [[nodiscard]] Parameter* parameter() const { return this->parameter_; }

  [[nodiscard]] Problem* problem() const { return this->problem_; }

  /** @brief O0..O4 顺序固定 */
  [[nodiscard]] const std::vector<Order*>& orders() const { return this->orders_; }

  /** @brief V0..V2 顺序固定 */
  [[nodiscard]] const std::vector<Vehicle*>& vehicles() const { return this->vehicles_; }

  /** @brief 受管站点 L1..L6(locations()[i]->ind == i + 1), 不含 default 站点 */
  [[nodiscard]] const std::vector<Location*>& managed_locations() const {
    return this->managed_locations_;
  }

 private:
  /** @brief 构造一个 weight=weight 的单明细订单 */
  [[nodiscard]] Order* make_order(int order_ind, Location* pick_loc, Location* drop_loc,
                                  std::int64_t weight);

  Scenario* scenario_{nullptr};
  DimensionManager* dim_manager_{nullptr};
  LabelManager* label_manager_{nullptr};
  LocationManager* location_manager_{nullptr};
  CarrierManager* carrier_manager_{nullptr};
  CargoOrderManager* cargo_order_manager_{nullptr};
  DistMatrixManager* dist_matrix_manager_{nullptr};
  VehicleModelManager* vehicle_model_manager_{nullptr};
  Carrier* carrier_{nullptr};

  /** @brief 所有站点共享的可用车辆集合, 由本夹具持有 */
  AvailableVehicle* available_vehicle_{nullptr};

  Parameter* parameter_{nullptr};
  Problem* problem_{nullptr};
  LoadEditor* load_editor_{nullptr};

  std::vector<Location*> managed_locations_;
  std::vector<Vehicle*> vehicles_;
  std::vector<Order*> orders_;
};

inline AlnsRepairFixture::AlnsRepairFixture() {
  // 矩阵长度 = default(ind 0) + L1..L6(ind 1..6)
  constexpr int kMatrixLen = kManagedLocationCount + 1;
  constexpr int kLocationPoolSize = kManagedLocationCount;

  this->scenario_ = new Scenario{};

  this->dim_manager_ = new DimensionManager();
  this->label_manager_ = new LabelManager();
  this->location_manager_ =
      new LocationManager(this->label_manager_->location_labelset, this->dim_manager_);
  this->carrier_manager_ = new CarrierManager(this->label_manager_->carrier_labelset);
  this->cargo_order_manager_ =
      new CargoOrderManager(this->label_manager_->order_labelset, this->dim_manager_);
  this->dist_matrix_manager_ = new DistMatrixManager(kMatrixLen);
  this->vehicle_model_manager_ =
      new VehicleModelManager(this->dim_manager_, this->label_manager_->vehicle_model_labelset);

  this->scenario_->set_dim_manager(this->dim_manager_);
  this->scenario_->set_label_manager(this->label_manager_);
  this->scenario_->set_vehicle_model_manager(this->vehicle_model_manager_);
  this->scenario_->set_carrier_manager(this->carrier_manager_);
  this->scenario_->set_location_manager(this->location_manager_);
  this->scenario_->set_dist_matrix_manager(this->dist_matrix_manager_);
  this->scenario_->set_cargo_order_manager(this->cargo_order_manager_);

  this->dim_manager_->create_dimension("weight", "weight", 0);

  // 站点 L1..L6: LocationManager 构造函数里已经创建了 default(ind 0)
  for (int i = 0; i < kLocationPoolSize; ++i) {
    const std::string code = "L" + std::to_string(i + 1);
    this->managed_locations_.push_back(
        this->location_manager_->create_location(code, code, 0.0, 0.0));
  }

  // 距离矩阵: dist(i,j) = i==j ? 0 : i*10+j, time(i,j) = i==j ? 0 : i+2*j
  this->dist_matrix_manager_->create_dist_matrix_code("DM", "DM");
  for (int from = 0; from < kMatrixLen; ++from) {
    for (int to = 0; to < kMatrixLen; ++to) {
      const std::int64_t dist = (from == to) ? 0 : from * 10 + to;
      const std::int64_t time = (from == to) ? 0 : from + 2 * to;
      this->dist_matrix_manager_->set_dist_time("DM", from, to, dist, time);
    }
  }
  DistMatrixCode* dist_matrix_code = this->dist_matrix_manager_->get_dist_matrix_code("DM");
  DistMatrix* dist_matrix = this->dist_matrix_manager_->get_dist_matrix(dist_matrix_code);

  // 车型: 能力 weight=100
  VehicleModel* vehicle_model =
      this->vehicle_model_manager_->create_vehicle_model("VM", "VM", dist_matrix_code, dist_matrix);
  vehicle_model->dim_vals[0] = kVehicleCapacity;

  // 承运商 + 3 辆车(每辆 count=1), orig/dest 都必须是真实站点:
  // V0: L1->L2, V1: L3->L4, V2: L5->L6
  this->carrier_ = this->carrier_manager_->create_carrier("C0", "carrier");
  for (int i = 0; i < kVehicleCount; ++i) {
    this->vehicles_.push_back(this->carrier_manager_->create_vehicle(
        this->carrier_->code, vehicle_model, 1, this->managed_locations_[2 * i],
        this->managed_locations_[2 * i + 1]));
  }

  // 所有站点(含 default)都挂可用车辆集合, 覆盖全部车辆。
  // 注意: 必须在 OrderFactory::creat_order 之前设置(Order 构造会解引用 available_vehicle)。
  this->available_vehicle_ = this->carrier_manager_->full_available_vehicle();
  for (Location* location : this->location_manager_->locations) {
    location->set_available_vehicle(this->available_vehicle_);
  }

  // 参数: 计划时间范围 + 距离软约束(obj_val 非零且逐车次不同)
  const auto logger = std::make_shared<spdlog::logger>(
      "alns_repair_fixture", std::make_shared<spdlog::sinks::null_sink_mt>());
  this->parameter_ = new Parameter(logger);
  this->parameter_->plan_datetime_range = new PlanDatetimeRange();
  this->parameter_->pick_drop_pattern = PickDropPatternType::SPMD;
  this->parameter_->load_unload_policy = LoadUnloadPolicyType::FILO;
  this->parameter_->soft_constr_items.emplace_back(SoftConstrCode::Distance,
                                                   YamlNode(YAML::Load("dist_factor: 1.0")));

  this->problem_ = new Problem(this->scenario_, this->parameter_);
  this->load_editor_ = new LoadEditor(this->problem_);

  // 订单(全部 pick L5 = managed_locations_[4]):
  // O0 -> L2(10), O1 -> L4(10), O2 -> L6(10), O3 -> L3(20), O4 -> L1(30)
  Location* pick_loc = this->managed_locations_[4];
  this->orders_.push_back(this->make_order(0, pick_loc, this->managed_locations_[1], 10));
  this->orders_.push_back(this->make_order(1, pick_loc, this->managed_locations_[3], 10));
  this->orders_.push_back(this->make_order(2, pick_loc, this->managed_locations_[5], 10));
  this->orders_.push_back(this->make_order(3, pick_loc, this->managed_locations_[2], 20));
  this->orders_.push_back(this->make_order(4, pick_loc, this->managed_locations_[0], 30));
}

inline AlnsRepairFixture::~AlnsRepairFixture() {
  // 与 SolverContext::~SolverContext 一致的所有权顺序(无 visual_manager):
  // load_editor -> problem -> parameter -> 订单 -> 夹具私有数据 -> scenario
  delete this->load_editor_;
  this->load_editor_ = nullptr;
  delete this->problem_;
  this->problem_ = nullptr;
  delete this->parameter_;
  this->parameter_ = nullptr;
  for (Order* order : this->orders_) {
    delete order;
  }
  this->orders_.clear();
  delete this->available_vehicle_;
  this->available_vehicle_ = nullptr;
  // scenario 拥有 dim/label/vehicle_model/carrier/location/dist_matrix/cargo_order 管理器
  delete this->scenario_;
  this->scenario_ = nullptr;
  this->dim_manager_ = nullptr;
  this->label_manager_ = nullptr;
  this->location_manager_ = nullptr;
  this->carrier_manager_ = nullptr;
  this->cargo_order_manager_ = nullptr;
  this->dist_matrix_manager_ = nullptr;
  this->vehicle_model_manager_ = nullptr;
  this->carrier_ = nullptr;
}

inline Order* AlnsRepairFixture::make_order(const int order_ind, Location* pick_loc,
                                            Location* drop_loc, const std::int64_t weight) {
  const std::string suffix = std::to_string(order_ind);
  CargoOrder* cargo_order = this->cargo_order_manager_->create_cargo_order(
      "CO" + suffix, "co" + suffix, pick_loc, drop_loc);
  const TimeWindow time_window =
      this->parameter_->plan_datetime_range->create_default_time_window();
  cargo_order->set_pick_time_window(time_window);
  cargo_order->set_drop_time_window(time_window);

  std::vector<std::int64_t> dim_vals = this->dim_manager_->empty_dim_values();
  dim_vals[0] = weight;
  LabelsetValue* labelset_value = this->label_manager_->order_labelset->empty_labelset_value();
  LabelsetValueBitset::UPtr labelset_value_bitset =
      this->label_manager_->order_labelset->empty_labelset_value_bitset();
  cargo_order->create_cargo_sub_order("SUB" + suffix, "sub" + suffix, dim_vals, labelset_value,
                                      std::move(labelset_value_bitset), 1);

  std::vector<CargoOrder*> cargo_orders{cargo_order};
  return OrderFactory::creat_order(cargo_orders, order_ind, this->parameter_->plan_datetime_range,
                                   this->scenario_, &this->problem_->tw_pattern_registry);
}

inline std::unique_ptr<Solution> AlnsRepairFixture::make_solution() const {
  auto solution = std::make_unique<Solution>();
  solution->vehicle_resource = ResourceFactory::create_vehicle_resource(this->carrier_manager_);

  // 2 个初始车次, 每个 1 单; build 只做评估不占车, 占用由 add_load 完成。
  for (int i = 0; i < kAssignedOrderCount; ++i) {
    const Order* order = this->orders_[i];
    const std::vector<const Order*> orders{order};
    Load* load = this->load_editor_->build(orders, *solution->vehicle_resource);
    if (load == nullptr) {
      throw std::runtime_error("AlnsRepairFixture: initial load build returned nullptr");
    }
    if (load->is_infeasible()) {
      delete load;
      throw std::runtime_error("AlnsRepairFixture: initial load build is infeasible");
    }
    solution->add_load(load, orders);
  }

  // 3 个未指派订单 -> 3 个 segment
  for (int i = kAssignedOrderCount; i < kOrderCount; ++i) {
    solution->add_unassigned_order(this->orders_[i]);
  }

  return solution;
}
