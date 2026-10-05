/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <mutex>
#include <thread>
#include <unordered_map>

#include "bdm_vehicle.h"
#include "pdm_load_profile.h"
#include "pdm_node.h"
#include "pdm_parameter.h"
#include "pdm_time_window_cache.h"
#include "se_scenario.h"

/**
 * @brief load context
 */
class LoadContext {
 public:
  /**
   * @brief scenario
   */
  const Scenario* scenario;

  /**
   * @brief 规划时间段, 惰性创建线程缓存时透传给 NodeTimeWindowCache
   */
  const PlanDatetimeRange* plan_datetime_range;

  /**
   * @brief node_tw 缓存容量
   */
  const int node_tw_cache_capacity;

  /**
   * @brief 按线程隔离的时间窗缓存
   * @details 并行下所有 load 副本共享同一份 LoadContext(深拷贝只复制 context 指针),
   * 因此缓存必须按线程拆分, 否则所有 worker 抢同一把锁。每个线程一份后查找路径零同步。
   */
  std::unordered_map<std::thread::id, NodeTimeWindowCache*> thread_tw_caches;

  /**
   * @brief 只保护 thread_tw_caches 的注册(每线程仅一次), 不在热路径上
   */
  std::mutex thread_tw_cache_mutex;

  LoadContext(const Scenario* scenario, const Parameter* parameter);

  ~LoadContext();

  /**
   * @brief 取当前线程专属的时间窗缓存, 首次调用时创建并注册
   */
  NodeTimeWindowCache* get_node_time_window_cache();
};

/**
 * @brief load
 * @details a complete delivery plan for orders
 */
class Load {
 public:
  /**
   * @brief load context
   */
  LoadContext* context;
  /**
   * @brief dist matrix code
   */
  const DistMatrixCode* dist_matrix_code;
  /**
   * @brief vehicle
   */
  const Vehicle* vehicle;
  /**
   * @brief first node of the load
   */
  Node::UPtr first_node;
  /**
   * @brief last node of the load
   */
  Node* last_node;
  /**
   * @brief route profile of the load
   */
  LoadRouteProfile::UPtr route_profile;
  /**
   * @brief constraint profile of the load
   */
  LoadConstrProfile::UPtr constr_profile;

  explicit Load(LoadContext* context);

  explicit Load(const Load* other);

  virtual ~Load();

  void reset_route_profile() const;

  void change_vehicle(const Vehicle* _vehicle);

  [[nodiscard]] long get_total_dist() const;

  [[nodiscard]] double get_obj_val() const;

  [[nodiscard]] Bitset* get_available_vehicle_bitset();

  [[nodiscard]] bool is_infeasible() const;

  [[nodiscard]] bool is_feasible() const;

  [[nodiscard]] bool is_empty() const;

  void update_node_dist_time();

  void update_start_node_dist_time();

  void update_end_node_dist_time() const;

  void update_time_window() const;

  /**
   * @brief 从 load 中摘除 activities
   * @details activities 需按 order 成对给出(提+卸); 摘除后若 node 已无 activity,
   * 则同步摘除该 node(首尾哨兵除外)
   */
  void remove_activities(const std::vector<Activity*>& activities) const;

  [[nodiscard]] std::vector<Node*> unfold_node_linked() const;

  [[nodiscard]] TimeWindowPlan::VecUPtr infer_time_window_plans() const;

  virtual const std::vector<long>& get_peak_load_dims() = 0;

  virtual int get_pick_node_count() = 0;

  virtual int get_drop_node_count() = 0;

  virtual LabelsetValueBitset* get_pick_loc_labelset_value_bitset() = 0;

  virtual LabelsetValueBitset* get_drop_loc_labelset_value_bitset() = 0;

  virtual LabelsetValueBitset* get_order_labelset_value_bitset() = 0;

  virtual std::vector<const Order*> get_orders() = 0;

  [[nodiscard]] virtual int get_order_count() const = 0;

  [[nodiscard]] virtual int get_activity_count() const = 0;

 protected:
  [[nodiscard]] const std::vector<long>& get_peak_load_dims_sp() const;

  [[nodiscard]] const std::vector<long>& get_peak_load_dims_sd() const;

  [[nodiscard]] const std::vector<long>& get_peak_load_dims_mp() const;

  [[nodiscard]] int get_pick_node_count_sp() const;

  [[nodiscard]] int get_pick_node_count_mp() const;

  [[nodiscard]] int get_drop_node_count_sd() const;

  [[nodiscard]] int get_drop_node_count_md() const;

  [[nodiscard]] LabelsetValueBitset* get_pick_loc_labelset_value_bitset_sp() const;

  [[nodiscard]] LabelsetValueBitset* get_pick_loc_labelset_value_bitset_mp() const;

  [[nodiscard]] LabelsetValueBitset* get_drop_loc_labelset_value_bitset_sd() const;

  [[nodiscard]] LabelsetValueBitset* get_drop_loc_labelset_value_bitset_md() const;

  [[nodiscard]] LabelsetValueBitset* get_order_labelset_value_bitset_sp() const;

  [[nodiscard]] LabelsetValueBitset* get_order_labelset_value_bitset_sd() const;

  [[nodiscard]] LabelsetValueBitset* get_order_labelset_value_bitset_mp() const;
};

/**
 * @brief the load of SPMD VRP pattern
 */
class LoadSPMD : public Load {
 public:
  explicit LoadSPMD(LoadContext* context) : Load(context) {};

  explicit LoadSPMD(const Load* other) : Load(other) {};

  ~LoadSPMD() override;

  int get_pick_node_count() override;

  int get_drop_node_count() override;

  const std::vector<long>& get_peak_load_dims() override;

  LabelsetValueBitset* get_pick_loc_labelset_value_bitset() override;

  LabelsetValueBitset* get_drop_loc_labelset_value_bitset() override;

  LabelsetValueBitset* get_order_labelset_value_bitset() override;

  std::vector<const Order*> get_orders() override;

  [[nodiscard]] int get_order_count() const override;

  [[nodiscard]] int get_activity_count() const override;
};
