/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "bdm_vehicle.h"
#include "pdm_cache.h"
#include "pdm_cache_time_window.h"
#include "pdm_load_profile.h"
#include "pdm_node.h"
#include "pdm_parameter.h"
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
   * @brief 规划时间段
   */
  const PlanDatetimeRange* plan_datetime_range;

  /**
   * @brief 缓存
   * @details 同一请求的所有工作线程共享这一份: 并行阶段只读, 新增条目先落在任务私有的
   * StoreBuffer 上, 屏障之后由单线程归档进本表。
   */
  Cache* cache;

  LoadContext(const Scenario* scenario, const Parameter* parameter);

  ~LoadContext();
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

  /**
   * @brief 任务私有的缓存写缓冲, 未挂载时为 nullptr(串行阶段直接读写冻结表)
   * @details 并行任务体创建 load 副本后挂载自己的写缓冲: 副本内部的重算结果落在这里,
   * 不触碰共享的冻结表。副本进入 solution 之前必须归档并清空。
   * 本字段不被深拷贝继承: 新副本的写缓冲由当前任务显式挂载。
   */
  CacheStoreBuffer* store_buffer{nullptr};

  explicit Load(LoadContext* context);

  explicit Load(const Load* other);

  virtual ~Load();

  void reset_route_profile() const;

  void change_vehicle(const Vehicle* _vehicle);

  [[nodiscard]] long get_total_travel_dist() const;

  [[nodiscard]] long get_total_travel_time() const;

  [[nodiscard]] double get_obj_val() const;

  [[nodiscard]] Bitset* get_available_vehicle_bitset() const;

  [[nodiscard]] bool is_infeasible() const;

  [[nodiscard]] bool is_feasible() const;

  [[nodiscard]] bool is_empty() const;

  [[nodiscard]] NodeTimeWindowStoreBuffer* get_node_tw_store_buffer() const;

  void update_node_dist_time() const;

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

  void fill_locations(std::vector<const Location*>& locations) const;

  [[nodiscard]] virtual const std::vector<long>& get_peak_load_dims() const = 0;

  [[nodiscard]] virtual const std::vector<long>& get_total_load_dims() const = 0;

  virtual int get_pick_node_count() = 0;

  virtual int get_drop_node_count() = 0;

  virtual LabelsetValueBitset* get_pick_loc_labelset_value_bitset() = 0;

  virtual LabelsetValueBitset* get_drop_loc_labelset_value_bitset() = 0;

  virtual LabelsetValueBitset* get_order_labelset_value_bitset() = 0;

  virtual std::vector<const Order*> get_orders() = 0;

  virtual void fill_orders(std::vector<const Order*>& orders) = 0;

  virtual std::vector<Activity*> get_pick_activities() = 0;

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

  [[nodiscard]] const std::vector<long>& get_peak_load_dims() const override;

  [[nodiscard]] const std::vector<long>& get_total_load_dims() const override;

  LabelsetValueBitset* get_pick_loc_labelset_value_bitset() override;

  LabelsetValueBitset* get_drop_loc_labelset_value_bitset() override;

  LabelsetValueBitset* get_order_labelset_value_bitset() override;

  std::vector<const Order*> get_orders() override;

  void fill_orders(std::vector<const Order*>& orders) override;

  std::vector<Activity*> get_pick_activities() override;

  [[nodiscard]] int get_order_count() const override;

  [[nodiscard]] int get_activity_count() const override;
};
