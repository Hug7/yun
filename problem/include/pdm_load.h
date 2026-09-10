/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
   * @brief node time window cache
   */
  NodeTimeWindowCache* node_time_window_cache;

  LoadContext(const Scenario* scenario, const PlanDatetimeRange* plan_datetime_range);

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
   * @brief pervious dist matrix code
   */
  const DistMatrixCode* prev_dist_matrix_code;
  /**
   * @brief vehicle
   */
  Vehicle* vehicle;
  /**
   * @brief fisrt node of the load
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

  Load(LoadContext* context);

  void reset_route_profile();

  void change_vehicle(Vehicle* vehicle);

  long get_total_dist();

  Bitset* get_available_vehicle_bitset();

  void update_node_dist_time();

  void update_start_node_dist_time();

  void update_end_node_dist_time();

  void update_time_window();

  virtual ~Load();

  virtual const std::vector<long>& get_peak_load_dims() = 0;

  virtual int get_pick_node_count() = 0;

  virtual int get_drop_node_count() = 0;

  virtual LabelsetValueBitset* get_pick_loc_labelset_value_bitset() = 0;

  virtual LabelsetValueBitset* get_drop_loc_labelset_value_bitset() = 0;

  virtual LabelsetValueBitset* get_order_labelset_value_bitset() = 0;

 protected:
  const std::vector<long>& get_peak_load_dims_sp();

  const std::vector<long>& get_peak_load_dims_sd();

  const std::vector<long>& get_peak_load_dims_mp();

  int get_pick_node_count_sp();

  int get_pick_node_count_mp();

  int get_drop_node_count_sd();

  int get_drop_node_count_md();

  LabelsetValueBitset* get_pick_loc_labelset_value_bitset_sp();

  LabelsetValueBitset* get_pick_loc_labelset_value_bitset_mp();

  LabelsetValueBitset* get_drop_loc_labelset_value_bitset_sd();

  LabelsetValueBitset* get_drop_loc_labelset_value_bitset_md();

  LabelsetValueBitset* get_order_labelset_value_bitset_sp();

  LabelsetValueBitset* get_order_labelset_value_bitset_sd();

  LabelsetValueBitset* get_order_labelset_value_bitset_mp();
};

/**
 * @brief the load of SPMD VRP pattern
 */
class LoadSPMD : public Load {
 public:
  LoadSPMD(LoadContext* context) : Load(context) {};

  ~LoadSPMD() override;

  int get_pick_node_count() override;

  int get_drop_node_count() override;

  const std::vector<long>& get_peak_load_dims() override;

  LabelsetValueBitset* get_pick_loc_labelset_value_bitset() override;

  LabelsetValueBitset* get_drop_loc_labelset_value_bitset() override;

  LabelsetValueBitset* get_order_labelset_value_bitset() override;
};
