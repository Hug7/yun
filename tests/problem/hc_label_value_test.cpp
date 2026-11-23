/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hc_label_value.h"

#include <gtest/gtest.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/null_sink.h>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "bdm_carrier.h"
#include "bdm_dimension.h"
#include "bdm_label.h"
#include "bdm_location.h"
#include "c_yaml_utils.h"
#include "hc_constraint_code.h"
#include "hc_manager.h"
#include "pdm_infeasible.h"
#include "pdm_load.h"
#include "pdm_parameter.h"
#include "pdm_parameter_constraint.h"
#include "pdm_score.h"
#include "se_scenario.h"

namespace {

// Both Load constructors dereference their argument (pdm_load.cpp:27 / :39), so the
// fake needs a real, minimally-initialized LoadContext. The Scenario owns all managers
// and has raw pointers with no initializers (scenario/include/se_scenario.h:26-54),
// so zero-init with `new Scenario{}` and set exactly the managers LoadContext and
// LoadRouteProfile read: dim_manager, label_manager, location_manager, carrier_manager.
LoadContext* MakeContext() {
  auto* scenario = new Scenario{};
  auto* dim_manager = new DimensionManager();
  auto* label_manager = new LabelManager();
  auto* location_manager = new LocationManager(label_manager->location_labelset, dim_manager);
  auto* carrier_manager = new CarrierManager(label_manager->carrier_labelset);
  scenario->set_dim_manager(dim_manager);
  scenario->set_label_manager(label_manager);
  scenario->set_location_manager(location_manager);
  scenario->set_carrier_manager(carrier_manager);
  // Build an unregistered logger directly: spdlog::null_logger_mt() would register
  // a named logger and throw on the second MakeContext() call in this test suite.
  auto logger =
      std::make_shared<spdlog::logger>("test", std::make_shared<spdlog::sinks::null_sink_mt>());
  auto* parameter = new Parameter(logger);
  parameter->plan_datetime_range = new PlanDatetimeRange();
  return new LoadContext(scenario, parameter);
}

// Minimal Load stand-in: eval only uses the label value bitset accessors.
class FakeLoad : public Load {
 public:
  explicit FakeLoad(LabelsetValueBitset::UPtr order_bitset,
                    LabelsetValueBitset::UPtr pick_bitset = nullptr,
                    LabelsetValueBitset::UPtr drop_bitset = nullptr)
      : Load(MakeContext()),
        order_bitset_(std::move(order_bitset)),
        pick_bitset_(std::move(pick_bitset)),
        drop_bitset_(std::move(drop_bitset)) {}

  const std::vector<std::int64_t>& get_peak_load_dims() const override { return dims_; }
  const std::vector<std::int64_t>& get_total_load_dims() const override { return dims_; }
  int get_pick_node_count() override { return 0; }
  int get_drop_node_count() override { return 0; }
  LabelsetValueBitset* get_pick_loc_labelset_value_bitset() override {
    return this->pick_bitset_.get();
  }
  LabelsetValueBitset* get_drop_loc_labelset_value_bitset() override {
    return this->drop_bitset_.get();
  }
  LabelsetValueBitset* get_order_labelset_value_bitset() override {
    return this->order_bitset_.get();
  }
  std::vector<const Order*> get_orders() override { return {}; }
  void fill_orders(std::vector<const Order*>& orders) override { orders.clear(); }
  std::vector<Activity*> get_pick_activities() override { return {}; }
  int get_order_count() const override { return 0; }
  int get_activity_count() const override { return 0; }

 private:
  std::vector<std::int64_t> dims_;
  LabelsetValueBitset::UPtr order_bitset_;
  LabelsetValueBitset::UPtr pick_bitset_;
  LabelsetValueBitset::UPtr drop_bitset_;
};

std::unique_ptr<Labelset> MakeLabelset(const std::string& code,
                                       std::initializer_list<const char*> values) {
  auto labelset = std::make_unique<Labelset>();
  auto* label = new Label(code, code, 0);
  for (const char* value : values) {
    label->add_label_value(value);
  }
  labelset->add_label(label);
  return labelset;
}

// temperature: cold=0, ambient=1, dangerous=2, fragile=3.
std::unique_ptr<Labelset> MakeTemperatureLabelset() {
  return MakeLabelset("temperature", {"cold", "ambient", "dangerous", "fragile"});
}

// region: north=0, south=1, east=2, west=3.
std::unique_ptr<Labelset> MakeRegionLabelset() {
  return MakeLabelset("region", {"north", "south", "east", "west"});
}

// big: v0=0 ... v129=129, used to exercise multi-word label bitmaps.
std::unique_ptr<Labelset> MakeBigLabelset() {
  auto labelset = std::make_unique<Labelset>();
  auto* label = new Label("big", "big", 0);
  for (int value_ind = 0; value_ind < 130; ++value_ind) {
    label->add_label_value("v" + std::to_string(value_ind));
  }
  labelset->add_label(label);
  return labelset;
}

void SetValues(LabelsetValueBitset& bitset, int label_ind, const std::vector<int>& values) {
  for (int value_ind : values) {
    bitset.set(label_ind, value_ind);
  }
}

HardConstrScore::UPtr EvalExclusiveOrder(const std::vector<std::vector<int>>& groups,
                                         const std::vector<int>& values) {
  auto labelset = MakeTemperatureLabelset();
  auto bitset = labelset->empty_labelset_value_bitset();
  SetValues(*bitset, 0, values);
  HcLabelValueExclusive constraint({labelset.get(), 0, LabelValueEntity::Order}, groups);
  FakeLoad load(std::move(bitset));
  return constraint.eval(&load);
}

HardConstrScore::UPtr EvalCompatibleOrder(const std::vector<std::vector<int>>& groups,
                                          const std::vector<int>& values) {
  auto labelset = MakeTemperatureLabelset();
  auto bitset = labelset->empty_labelset_value_bitset();
  SetValues(*bitset, 0, values);
  HcLabelValueCompatible constraint({labelset.get(), 0, LabelValueEntity::Order}, groups);
  FakeLoad load(std::move(bitset));
  return constraint.eval(&load);
}

HardConstrScore::UPtr EvalExclusiveLocation(const std::vector<std::vector<int>>& groups,
                                            const std::vector<int>& pick_values,
                                            const std::vector<int>& drop_values,
                                            LabelValueSide side = LabelValueSide::Both) {
  auto labelset = MakeRegionLabelset();
  auto pick_bitset = labelset->empty_labelset_value_bitset();
  auto drop_bitset = labelset->empty_labelset_value_bitset();
  SetValues(*pick_bitset, 0, pick_values);
  SetValues(*drop_bitset, 0, drop_values);
  HcLabelValueExclusive constraint({labelset.get(), 0, LabelValueEntity::Location, side}, groups);
  FakeLoad load(nullptr, std::move(pick_bitset), std::move(drop_bitset));
  return constraint.eval(&load);
}

HardConstrScore::UPtr EvalCompatibleLocation(const std::vector<std::vector<int>>& groups,
                                             const std::vector<int>& pick_values,
                                             const std::vector<int>& drop_values,
                                             LabelValueSide side = LabelValueSide::Both) {
  auto labelset = MakeRegionLabelset();
  auto pick_bitset = labelset->empty_labelset_value_bitset();
  auto drop_bitset = labelset->empty_labelset_value_bitset();
  SetValues(*pick_bitset, 0, pick_values);
  SetValues(*drop_bitset, 0, drop_values);
  HcLabelValueCompatible constraint({labelset.get(), 0, LabelValueEntity::Location, side}, groups);
  FakeLoad load(nullptr, std::move(pick_bitset), std::move(drop_bitset));
  return constraint.eval(&load);
}

// Scenario has raw owning pointers with no initializers; zero-init with {} before use.
std::unique_ptr<Scenario> MakeScenarioWithLabels() {
  auto scenario = std::unique_ptr<Scenario>(new Scenario{});
  auto* label_manager = new LabelManager();
  scenario->set_label_manager(label_manager);

  auto* order_label = new Label("temperature", "温度", 0);
  for (const char* value : {"cold", "ambient", "dangerous", "fragile"}) {
    order_label->add_label_value(value);
  }
  label_manager->order_labelset->add_label(order_label);

  auto* location_label = new Label("region", "区域", 0);
  for (const char* value : {"north", "south", "east", "west"}) {
    location_label->add_label_value(value);
  }
  label_manager->location_labelset->add_label(location_label);
  return scenario;
}

HardConstrItem MakeItem(HardConstrCode code, const std::string& params_yaml) {
  return HardConstrItem(code, YamlNode(YAML::Load(params_yaml)));
}

}  // namespace

TEST(HcLabelValueReasonTest, ReasonCodesAreUnique) {
  const std::vector<const InfeasibleReason*> reasons = {
      &InfeasibleReasonCollection::FEASIBLE,
      &InfeasibleReasonCollection::UNDEFINED,
      &InfeasibleReasonCollection::VEHICLE_RESOURCE,
      &InfeasibleReasonCollection::MAX_PICK_NODE_COUNT,
      &InfeasibleReasonCollection::MAX_DROP_NODE_COUNT,
      &InfeasibleReasonCollection::VEHICLE_CAPACITY,
      &InfeasibleReasonCollection::AVAILABLE_VEHICLE,
      &InfeasibleReasonCollection::DISTINCT_LABEL_VALUE_MAX_COUNT,
      &InfeasibleReasonCollection::FILO_LOAD_UNLOAD_POLICY,
      &InfeasibleReasonCollection::FIFO_LOAD_UNLOAD_POLICY,
      &InfeasibleReasonCollection::TIME_WINDOW,
      &InfeasibleReasonCollection::NO_CHARGE_RATE,
      &InfeasibleReasonCollection::ORDER_LABEL_VALUE_EXCLUSIVE,
      &InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_EXCLUSIVE,
      &InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_EXCLUSIVE,
      &InfeasibleReasonCollection::ORDER_LABEL_VALUE_COMPATIBLE,
      &InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_COMPATIBLE,
      &InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_COMPATIBLE,
  };
  std::unordered_set<std::string> codes;
  codes.reserve(reasons.size());
  for (const auto* reason : reasons) {
    EXPECT_TRUE(codes.insert(reason->code).second) << "duplicate code " << reason->code;
  }
  EXPECT_EQ(InfeasibleReasonCollection::ORDER_LABEL_VALUE_EXCLUSIVE.code, "1012");
  EXPECT_EQ(InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_EXCLUSIVE.code, "1013");
  EXPECT_EQ(InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_EXCLUSIVE.code, "1014");
  EXPECT_EQ(InfeasibleReasonCollection::ORDER_LABEL_VALUE_COMPATIBLE.code, "1015");
  EXPECT_EQ(InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_COMPATIBLE.code, "1016");
  EXPECT_EQ(InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_COMPATIBLE.code, "1017");
}

TEST(HcLabelValueCodeTest, EnumRoundTrip) {
  for (const char* name : {"LabelValueExclusive", "LabelValueCompatible"}) {
    const auto code = magic_enum::enum_cast<HardConstrCode>(name);
    ASSERT_TRUE(code.has_value()) << name;
    EXPECT_EQ(magic_enum::enum_name(*code), name);
  }
}

TEST(HcLabelValueExclusiveTest, OrderSameGroupIsInfeasible) {
  const auto score = EvalExclusiveOrder({{0, 1}, {2, 3}}, {0, 1});
  EXPECT_TRUE(score->is_infeasible());
  EXPECT_EQ(score->code, HardConstrCode::LabelValueExclusive);
  EXPECT_EQ(score->reason.code, InfeasibleReasonCollection::ORDER_LABEL_VALUE_EXCLUSIVE.code);
  EXPECT_DOUBLE_EQ(score->get_score(), 1.0);
}

TEST(HcLabelValueExclusiveTest, OrderDifferentGroupsIsFeasible) {
  const auto score = EvalExclusiveOrder({{0, 1}, {2, 3}}, {0, 2});
  EXPECT_TRUE(score->is_feasible());
  EXPECT_EQ(score->reason.code, InfeasibleReasonCollection::FEASIBLE.code);
}

TEST(HcLabelValueExclusiveTest, OrderOverlappingGroupsAreNotMerged) {
  // [cold, ambient, dangerous] and [dangerous, fragile]: dangerous/fragile are
  // exclusive, but cold/fragile and ambient/fragile are allowed.
  const auto cold_fragile = EvalExclusiveOrder({{0, 1, 2}, {2, 3}}, {0, 3});
  EXPECT_TRUE(cold_fragile->is_feasible());

  const auto ambient_fragile = EvalExclusiveOrder({{0, 1, 2}, {2, 3}}, {1, 3});
  EXPECT_TRUE(ambient_fragile->is_feasible());

  const auto dangerous_fragile = EvalExclusiveOrder({{0, 1, 2}, {2, 3}}, {2, 3});
  EXPECT_TRUE(dangerous_fragile->is_infeasible());

  const auto cold_dangerous = EvalExclusiveOrder({{0, 1, 2}, {2, 3}}, {0, 2});
  EXPECT_TRUE(cold_dangerous->is_infeasible());
}

TEST(HcLabelValueExclusiveTest, OrderUnlistedValuesAreIgnored) {
  const auto score = EvalExclusiveOrder({{0, 1}}, {2, 3});
  EXPECT_TRUE(score->is_feasible());
}

TEST(HcLabelValueExclusiveTest, OrderSingleValueIsFeasible) {
  const auto score = EvalExclusiveOrder({{0}, {1}}, {0});
  EXPECT_TRUE(score->is_feasible());
}

TEST(HcLabelValueExclusiveTest, OrderOtherLabelDoesNotInterfere) {
  auto labelset = MakeTemperatureLabelset();
  auto* other = new Label("route_type", "线路类型", 1);
  other->add_label_value("urban");
  labelset->add_label(other);

  auto bitset = labelset->empty_labelset_value_bitset();
  bitset->set(1, 0);  // other label's value
  bitset->set(0, 0);  // cold only
  HcLabelValueExclusive constraint({labelset.get(), 0, LabelValueEntity::Order}, {{0, 1}, {2, 3}});
  FakeLoad load(std::move(bitset));
  EXPECT_TRUE(constraint.eval(&load)->is_feasible());
}

TEST(HcLabelValueExclusiveTest, CrossWordGroups) {
  auto labelset = MakeBigLabelset();
  const auto eval = [&](const std::vector<std::vector<int>>& groups,
                        const std::vector<int>& values) {
    auto bitset = labelset->empty_labelset_value_bitset();
    SetValues(*bitset, 0, values);
    HcLabelValueExclusive constraint({labelset.get(), 0, LabelValueEntity::Order}, groups);
    FakeLoad load(std::move(bitset));
    return constraint.eval(&load);
  };

  EXPECT_TRUE(eval({{0, 64, 128}}, {0, 128})->is_infeasible());
  EXPECT_TRUE(eval({{0, 64}, {65, 129}}, {0, 65})->is_feasible());
  EXPECT_TRUE(eval({{0, 64}, {65, 129}}, {0, 64})->is_infeasible());
  EXPECT_TRUE(eval({{0, 65}, {64, 129}}, {64, 129})->is_infeasible());
  EXPECT_TRUE(eval({{0, 65}, {64, 129}}, {0, 64})->is_feasible());
}

TEST(HcLabelValueCompatibleTest, OrderSameGroupIsFeasible) {
  const auto score = EvalCompatibleOrder({{0, 1}, {2, 3}}, {0, 1});
  EXPECT_TRUE(score->is_feasible());
  EXPECT_EQ(score->code, HardConstrCode::LabelValueCompatible);
}

TEST(HcLabelValueCompatibleTest, OrderDifferentGroupsIsInfeasible) {
  const auto score = EvalCompatibleOrder({{0, 1}, {2, 3}}, {0, 2});
  EXPECT_TRUE(score->is_infeasible());
  EXPECT_EQ(score->reason.code, InfeasibleReasonCollection::ORDER_LABEL_VALUE_COMPATIBLE.code);
}

TEST(HcLabelValueCompatibleTest, OrderUnlistedValuesFormDefaultGroup) {
  // dangerous and fragile are unlisted together, so they can share a load.
  const auto score = EvalCompatibleOrder({{0, 1}}, {2, 3});
  EXPECT_TRUE(score->is_feasible());
}

TEST(HcLabelValueCompatibleTest, OrderListedAndUnlistedIsInfeasible) {
  const auto score = EvalCompatibleOrder({{0, 1}}, {0, 2});
  EXPECT_TRUE(score->is_infeasible());
}

TEST(HcLabelValueCompatibleTest, OrderAllValuesListedHasNoDefaultGroup) {
  auto labelset = MakeTemperatureLabelset();
  HcLabelValueCompatible constraint({labelset.get(), 0, LabelValueEntity::Order}, {{0, 1}, {2, 3}});
  ASSERT_EQ(constraint.groups.size(), 2u);
  EXPECT_EQ(constraint.groups[0], (std::vector<int>{0, 1}));
  EXPECT_EQ(constraint.groups[1], (std::vector<int>{2, 3}));
}

TEST(HcLabelValueExclusiveTest, LocationPickViolationIsInfeasible) {
  const auto score = EvalExclusiveLocation({{0, 1}, {2, 3}}, {0, 1}, {});
  EXPECT_TRUE(score->is_infeasible());
  EXPECT_EQ(score->reason.code,
            InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_EXCLUSIVE.code);
}

TEST(HcLabelValueExclusiveTest, LocationDropViolationIsInfeasible) {
  const auto score = EvalExclusiveLocation({{0, 1}, {2, 3}}, {}, {0, 1});
  EXPECT_TRUE(score->is_infeasible());
  EXPECT_EQ(score->reason.code,
            InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_EXCLUSIVE.code);
}

TEST(HcLabelValueExclusiveTest, LocationCrossSideIsNotJudged) {
  const auto score = EvalExclusiveLocation({{0, 1}, {2, 3}}, {0}, {2});
  EXPECT_TRUE(score->is_feasible());
}

TEST(HcLabelValueExclusiveTest, LocationPickSideIgnoresDropViolation) {
  const auto score = EvalExclusiveLocation({{0, 1}, {2, 3}}, {0}, {0, 1}, LabelValueSide::Pick);
  EXPECT_TRUE(score->is_feasible());
}

TEST(HcLabelValueExclusiveTest, LocationPickSideChecksPickViolation) {
  const auto score = EvalExclusiveLocation({{0, 1}, {2, 3}}, {0, 1}, {}, LabelValueSide::Pick);
  EXPECT_TRUE(score->is_infeasible());
}

TEST(HcLabelValueExclusiveTest, LocationDropSideIgnoresPickViolation) {
  const auto score = EvalExclusiveLocation({{0, 1}, {2, 3}}, {0, 1}, {2}, LabelValueSide::Drop);
  EXPECT_TRUE(score->is_feasible());
}

TEST(HcLabelValueExclusiveTest, LocationDropSideChecksDropViolation) {
  const auto score = EvalExclusiveLocation({{0, 1}, {2, 3}}, {}, {0, 1}, LabelValueSide::Drop);
  EXPECT_TRUE(score->is_infeasible());
}

TEST(HcLabelValueCompatibleTest, CrossWordDefaultGroup) {
  auto labelset = MakeBigLabelset();
  const auto eval = [&](const std::vector<std::vector<int>>& groups,
                        const std::vector<int>& values) {
    auto bitset = labelset->empty_labelset_value_bitset();
    SetValues(*bitset, 0, values);
    HcLabelValueCompatible constraint({labelset.get(), 0, LabelValueEntity::Order}, groups);
    FakeLoad load(std::move(bitset));
    return constraint.eval(&load);
  };

  // Explicit groups {0, 64} and {65, 129}; the remaining values form the default group.
  EXPECT_TRUE(eval({{0, 64}, {65, 129}}, {0, 64})->is_feasible());
  EXPECT_TRUE(eval({{0, 64}, {65, 129}}, {0, 65})->is_infeasible());
  EXPECT_TRUE(eval({{0, 64}, {65, 129}}, {1, 2})->is_feasible());
  EXPECT_TRUE(eval({{0, 64}, {65, 129}}, {128, 1})->is_feasible());
  EXPECT_TRUE(eval({{0, 64}, {65, 129}}, {0, 1})->is_infeasible());
}

TEST(HcLabelValueCompatibleTest, LocationPickViolationIsInfeasible) {
  const auto score = EvalCompatibleLocation({{0, 1}, {2, 3}}, {0, 2}, {});
  EXPECT_TRUE(score->is_infeasible());
  EXPECT_EQ(score->reason.code,
            InfeasibleReasonCollection::PICK_LOCATION_LABEL_VALUE_COMPATIBLE.code);
}

TEST(HcLabelValueCompatibleTest, LocationCrossSideIsNotJudged) {
  const auto score = EvalCompatibleLocation({{0, 1}, {2, 3}}, {0}, {2});
  EXPECT_TRUE(score->is_feasible());
}

TEST(HcLabelValueCompatibleTest, LocationPickSideIgnoresDropViolation) {
  const auto score = EvalCompatibleLocation({{0, 1}, {2, 3}}, {0}, {0, 2}, LabelValueSide::Pick);
  EXPECT_TRUE(score->is_feasible());
}

TEST(HcLabelValueCompatibleTest, LocationDropSideChecksDropViolation) {
  const auto score = EvalCompatibleLocation({{0, 1}, {2, 3}}, {}, {0, 2}, LabelValueSide::Drop);
  EXPECT_TRUE(score->is_infeasible());
  EXPECT_EQ(score->reason.code,
            InfeasibleReasonCollection::DROP_LOCATION_LABEL_VALUE_COMPATIBLE.code);
}

TEST(HcLabelValueParserTest, ExclusiveParsesGroups) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: temperature\nentity: Order\n"
                       "groups: [[cold, ambient], [ambient, dangerous]]");
  std::unique_ptr<HcLabelValueExclusive> constraint(
      HardConstrParser::label_value_exclusive(scenario.get(), item));
  ASSERT_NE(constraint, nullptr);
  EXPECT_EQ(constraint->scope.label_ind, 0);
  EXPECT_EQ(constraint->scope.entity, LabelValueEntity::Order);
  ASSERT_EQ(constraint->groups.size(), 2u);
  EXPECT_EQ(constraint->groups[0], (std::vector<int>{0, 1}));
  EXPECT_EQ(constraint->groups[1], (std::vector<int>{1, 2}));
}

TEST(HcLabelValueParserTest, ParsesFromHardConstraintsYaml) {
  const auto root =
      YamlNode(YAML::Load("hard_constraints:\n"
                          "  - code: LabelValueExclusive\n"
                          "    params:\n"
                          "      label_code: temperature\n"
                          "      entity: Order\n"
                          "      groups: [[cold, ambient]]\n"));
  const auto items = parse_hard_constr_items(root.get_yaml_node("hard_constraints"));
  ASSERT_EQ(items.size(), 1u);
  EXPECT_EQ(items[0].code, HardConstrCode::LabelValueExclusive);

  auto scenario = MakeScenarioWithLabels();
  std::unique_ptr<HcLabelValueExclusive> constraint(
      HardConstrParser::label_value_exclusive(scenario.get(), items[0]));
  ASSERT_NE(constraint, nullptr);
  EXPECT_EQ(constraint->scope.label_ind, 0);
  ASSERT_EQ(constraint->groups.size(), 1u);
  EXPECT_EQ(constraint->groups[0], (std::vector<int>{0, 1}));
}

TEST(HcLabelValueParserTest, CompatibleAppendsUnlistedDefaultGroup) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueCompatible,
                       "label_code: temperature\nentity: Order\ngroups: [[cold, ambient]]");
  std::unique_ptr<HcLabelValueCompatible> constraint(
      HardConstrParser::label_value_compatible(scenario.get(), item));
  ASSERT_NE(constraint, nullptr);
  ASSERT_EQ(constraint->groups.size(), 2u);
  EXPECT_EQ(constraint->groups[0], (std::vector<int>{0, 1}));
  EXPECT_EQ(constraint->groups[1], (std::vector<int>{2, 3}));
}

TEST(HcLabelValueParserTest, LocationEntityUsesLocationLabelset) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: region\nentity: Location\ngroups: [[north, south]]");
  std::unique_ptr<HcLabelValueExclusive> constraint(
      HardConstrParser::label_value_exclusive(scenario.get(), item));
  ASSERT_NE(constraint, nullptr);
  EXPECT_EQ(constraint->scope.label_ind, 0);
  EXPECT_EQ(constraint->scope.entity, LabelValueEntity::Location);
  EXPECT_EQ(constraint->scope.side, LabelValueSide::Both);
  EXPECT_EQ(constraint->scope.labelset, scenario->label_manager->location_labelset);
}

TEST(HcLabelValueParserTest, ParsesLocationSide) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: region\nentity: Location\nside: Pick\n"
                       "groups: [[north, south]]");
  std::unique_ptr<HcLabelValueExclusive> constraint(
      HardConstrParser::label_value_exclusive(scenario.get(), item));
  ASSERT_NE(constraint, nullptr);
  EXPECT_EQ(constraint->scope.side, LabelValueSide::Pick);
}

TEST(HcLabelValueParserTest, ThrowsOnInvalidLocationSide) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: region\nentity: Location\nside: Middle\n"
                       "groups: [[north, south]]");
  EXPECT_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueParserTest, ThrowsOnSideForOrder) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: temperature\nentity: Order\nside: Pick\n"
                       "groups: [[cold, ambient]]");
  EXPECT_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueParserTest, ThrowsOnUnknownEntity) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: temperature\nentity: Vehicle\ngroups: [[cold]]");
  EXPECT_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueParserTest, ThrowsOnUnknownLabelCode) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: nonexistent\nentity: Order\ngroups: [[cold]]");
  EXPECT_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueParserTest, ThrowsOnUnknownValueCode) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: temperature\nentity: Order\ngroups: [[cold, nonexistent]]");
  EXPECT_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueParserTest, ThrowsOnEmptyGroups) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: temperature\nentity: Order\ngroups: []");
  EXPECT_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueParserTest, ThrowsOnEmptyGroup) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: temperature\nentity: Order\ngroups: [[]]");
  EXPECT_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueParserTest, ThrowsOnDuplicateValueInSameGroup) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: temperature\nentity: Order\ngroups: [[cold, cold]]");
  EXPECT_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueParserTest, ThrowsOnCompatibleOverlapAcrossGroups) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueCompatible,
                       "label_code: temperature\nentity: Order\n"
                       "groups: [[cold, ambient], [ambient, dangerous]]");
  EXPECT_THROW(HardConstrParser::label_value_compatible(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueParserTest, ExclusiveOverlapAcrossGroupsIsAllowed) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: temperature\nentity: Order\n"
                       "groups: [[cold, ambient], [ambient, dangerous]]");
  EXPECT_NO_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item));
}

TEST(HcLabelValueParserTest, ThrowsOnMalformedGroups) {
  auto scenario = MakeScenarioWithLabels();
  auto item = MakeItem(HardConstrCode::LabelValueExclusive,
                       "label_code: temperature\nentity: Order\ngroups: [cold, ambient]");
  EXPECT_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueParserTest, ThrowsOnMissingEntity) {
  auto scenario = MakeScenarioWithLabels();
  auto item =
      MakeItem(HardConstrCode::LabelValueExclusive, "label_code: temperature\ngroups: [[cold]]");
  EXPECT_THROW(HardConstrParser::label_value_exclusive(scenario.get(), item), std::runtime_error);
}

TEST(HcLabelValueManagerTest, RegistersConfiguredConstraints) {
  auto scenario = MakeScenarioWithLabels();
  auto logger =
      std::make_shared<spdlog::logger>("test", std::make_shared<spdlog::sinks::null_sink_mt>());
  Parameter parameter(logger);
  parameter.hard_constr_items.emplace_back(
      HardConstrCode::LabelValueExclusive,
      YamlNode(YAML::Load("label_code: temperature\nentity: Order\ngroups: [[cold, ambient]]")));
  parameter.hard_constr_items.emplace_back(
      HardConstrCode::LabelValueCompatible,
      YamlNode(YAML::Load("label_code: region\nentity: Location\ngroups: [[north, south]]")));

  HardConstraintManager manager(scenario.get(), &parameter);
  bool has_exclusive = false;
  bool has_compatible = false;
  for (const HardConstraint* constraint : manager.constrs) {
    has_exclusive |= constraint->code == HardConstrCode::LabelValueExclusive;
    has_compatible |= constraint->code == HardConstrCode::LabelValueCompatible;
  }
  EXPECT_TRUE(has_exclusive);
  EXPECT_TRUE(has_compatible);
}
