/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "bdm_charge.h"
#include "scr_charge_reader.h"

namespace {

/**
 * @brief 测试用静态计费视图
 * @details 直接把测试数据返回给 ChargeManager，不做懒加载
 */
class StaticChargeLoadView final : public ChargeLoadView {
 public:
  StaticChargeLoadView(const int vehicle_ind, const long distance_m, const int pick_node_count,
                       const int drop_node_count, const long* dim_vals,
                       const Location* const* stops, const int stop_count,
                       const Location* const* order_pick_locs,
                       const Location* const* order_drop_locs, const int order_count)
      : vehicle_ind_(vehicle_ind),
        distance_m_(distance_m),
        pick_node_count_(pick_node_count),
        drop_node_count_(drop_node_count),
        dim_vals_(dim_vals),
        stops_(stops),
        stop_count_(stop_count),
        order_pick_locs_(order_pick_locs),
        order_drop_locs_(order_drop_locs),
        order_count_(order_count) {}

  mutable int vehicle_calls{0};
  mutable int distance_calls{0};
  mutable int pick_count_calls{0};
  mutable int drop_count_calls{0};
  mutable int dims_calls{0};
  mutable int stops_calls{0};
  mutable int stop_count_calls{0};
  mutable int order_pick_calls{0};
  mutable int order_drop_calls{0};
  mutable int order_count_calls{0};

  [[nodiscard]] int vehicle_ind() const override {
    ++vehicle_calls;
    return vehicle_ind_;
  }

  [[nodiscard]] long distance_m() const override {
    ++distance_calls;
    return distance_m_;
  }

  [[nodiscard]] int pick_node_count() const override {
    ++pick_count_calls;
    return pick_node_count_;
  }

  [[nodiscard]] int drop_node_count() const override {
    ++drop_count_calls;
    return drop_node_count_;
  }

  [[nodiscard]] const long* dim_vals() const override {
    ++dims_calls;
    return dim_vals_;
  }

  [[nodiscard]] const Location* const* stops() const override {
    ++stops_calls;
    return stops_;
  }

  [[nodiscard]] int stop_count() const override {
    ++stop_count_calls;
    return stop_count_;
  }

  [[nodiscard]] const Location* const* order_pick_locs() const override {
    ++order_pick_calls;
    return order_pick_locs_;
  }

  [[nodiscard]] const Location* const* order_drop_locs() const override {
    ++order_drop_calls;
    return order_drop_locs_;
  }

  [[nodiscard]] int order_count() const override {
    ++order_count_calls;
    return order_count_;
  }

 private:
  int vehicle_ind_;
  long distance_m_;
  int pick_node_count_;
  int drop_node_count_;
  const long* dim_vals_;
  const Location* const* stops_;
  int stop_count_;
  const Location* const* order_pick_locs_;
  const Location* const* order_drop_locs_;
  int order_count_;
};

/**
 * @brief 计费测试场景
 * @details 构建最小可用的维度、标签、站点、承运商、车型与车辆
 */
class ChargeFixture {
 public:
  DimensionManager dims;
  LabelManager labels;
  LocationManager locations;
  CarrierManager carriers;
  VehicleModelManager models;
  ChargeManager charge;
  Label* zone{nullptr};
  Vehicle* vehicle{nullptr};

  ChargeFixture()
      : dims(),
        labels(),
        locations(labels.location_labelset, &dims),
        carriers(labels.carrier_labelset),
        models(&dims, labels.vehicle_model_labelset),
        charge() {
    dims.create_dimension("weight", "weight", 2);
    zone = labels.create_label("zone", "zone");
    zone->add_label_value("SH");
    zone->add_label_value("SZ");
    zone->add_label_value("WX");
    zone->add_label_value("HZ");
    labels.labelset_add_label("zone", "Location");

    this->create_location("SH");
    this->create_location("SZ");
    this->create_location("WX");
    this->create_location("HZ");

    carriers.create_carrier("C1", "C1");
    VehicleModel* vehicle_model = models.create_vehicle_model("M1", "M1", nullptr, nullptr);
    vehicle = carriers.create_vehicle("C1", vehicle_model, 1, locations.get_default_location(),
                                      locations.get_default_location());

    charge.charge_code_codes = {"TC1", "TC2"};
    charge.charge_code_names = {"TC1", "TC2"};
  }

  Location* create_location(const std::string& code) {
    Location* location = locations.create_location(code, code, 0, 0);
    location->add_label_value(0, zone->get_label_value(code));
    return location;
  }

  [[nodiscard]] int zone_value(const std::string& value) const {
    return zone->get_label_value(value)->ind;
  }

  void build() { charge.build_indexes(&dims, &labels, &carriers, &models, &locations); }

  [[nodiscard]] StaticChargeLoadView make_view(const long distance, const int pick, const int drop,
                                               const long* dim_vals, const Location* const* stops,
                                               const int stop_count,
                                               const Location* const* order_picks = nullptr,
                                               const Location* const* order_drops = nullptr,
                                               const int order_count = 0) const {
    return StaticChargeLoadView(vehicle->ind, distance, pick, drop, dim_vals, stops, stop_count,
                                order_picks, order_drops, order_count);
  }
};

/**
 * @brief 追加一条无线路、无条件的规则
 */
void add_flat_rule(ChargeFixture& fixture, const int charge_code_ind,
                   const std::string& secondary_code, const long price) {
  ChargeRule rule;
  rule.charge_code_ind = charge_code_ind;
  rule.secondary_ind = fixture.charge.add_secondary(charge_code_ind, secondary_code);
  rule.tier_begin = static_cast<int>(fixture.charge.tiers.size());
  rule.tier_len = 1;
  fixture.charge.rules.push_back(rule);
  fixture.charge.tiers.push_back(ChargeTier{0, std::numeric_limits<long>::max(),
                                            static_cast<int>(fixture.charge.items.size()), 1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, price, -1});
}

/**
 * @brief 追加一条 OD_PAIR 规则
 */
void add_od_rule(ChargeFixture& fixture, const int charge_code_ind,
                 const std::string& secondary_code, const std::string& from, const std::string& to,
                 const long price) {
  ChargeRule rule;
  rule.charge_code_ind = charge_code_ind;
  rule.secondary_ind = fixture.charge.add_secondary(charge_code_ind, secondary_code);
  rule.path_type = ChargePathType::OD_PAIR;
  rule.path_label_ind4labelset = 0;
  rule.path_values = {fixture.zone_value(from), fixture.zone_value(to)};
  rule.tier_begin = static_cast<int>(fixture.charge.tiers.size());
  rule.tier_len = 1;
  fixture.charge.rules.push_back(rule);
  fixture.charge.tiers.push_back(ChargeTier{0, std::numeric_limits<long>::max(),
                                            static_cast<int>(fixture.charge.items.size()), 1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, price, -1});
}

}  // namespace

TEST(ChargeManagerTest, OdPairWithDistanceTier) {
  ChargeFixture fixture;
  ChargeRule rule;
  rule.charge_code_ind = 0;
  rule.secondary_ind = fixture.charge.add_secondary(0, "BASE");
  rule.path_type = ChargePathType::OD_PAIR;
  rule.path_label_ind4labelset = 0;
  rule.path_values = {fixture.zone_value("SH"), fixture.zone_value("SZ")};
  rule.condition_type = ChargeConditionType::DISTANCE;
  rule.tier_begin = 0;
  rule.tier_len = 1;
  fixture.charge.rules.push_back(rule);
  fixture.charge.tiers.push_back(ChargeTier{0, 100000, 0, 2});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, 1000000, -1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::DISTANCE, 23500, -1});
  fixture.build();

  const Location* const stops[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SZ")};
  const Location* const picks[] = {fixture.locations.get_location("SH")};
  const Location* const drops[] = {fixture.locations.get_location("SZ")};
  // 100 元 + 2.35 元/km * 50 km = 217.5 元
  EXPECT_EQ(
      fixture.charge.evaluate(fixture.make_view(50000, 1, 1, nullptr, stops, 2, picks, drops, 1))
          .amount,
      2175000);
}

TEST(ChargeManagerTest, SecondarySumAndChargeCodeMin) {
  ChargeFixture fixture;
  // TC1/BASE = 100 + 10 = 110；TC1/FUEL = 30；TC1 = max(110, 30) = 110
  add_flat_rule(fixture, 0, "BASE", 1000000);
  add_flat_rule(fixture, 0, "BASE", 100000);
  add_flat_rule(fixture, 0, "FUEL", 300000);
  // TC2/BASE = 200；TC2 = 200；最终取 min(110, 200) = 110
  add_flat_rule(fixture, 1, "BASE", 2000000);
  fixture.build();

  const Location* const stops[] = {fixture.locations.get_location("SH")};
  EXPECT_EQ(fixture.charge.evaluate(fixture.make_view(0, 1, 1, nullptr, stops, 1)).amount, 1100000);
}

TEST(ChargeManagerTest, OrderedRouteAndExtraStopFee) {
  ChargeFixture fixture;
  ChargeRule rule;
  rule.charge_code_ind = 0;
  rule.secondary_ind = fixture.charge.add_secondary(0, "BASE");
  rule.path_type = ChargePathType::ORDERED_ROUTE;
  rule.path_label_ind4labelset = 0;
  rule.path_values = {fixture.zone_value("SH"), fixture.zone_value("SZ"), fixture.zone_value("WX")};
  rule.condition_type = ChargeConditionType::PICK_NODE_COUNT;
  rule.tier_begin = 0;
  rule.tier_len = 2;
  fixture.charge.rules.push_back(rule);
  fixture.charge.tiers.push_back(ChargeTier{0, 2, 0, 1});
  fixture.charge.tiers.push_back(ChargeTier{2, 4, 1, 2});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, 1000000, -1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, 1000000, -1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::PICK_STOP, 500000, -1});
  fixture.build();

  const Location* const stops[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SZ"),
                                   fixture.locations.get_location("WX")};
  // 3 个提货点：100 元 + 50 元 * (3-1) = 200 元
  EXPECT_EQ(fixture.charge.evaluate(fixture.make_view(0, 3, 1, nullptr, stops, 3)).amount, 2000000);
}

TEST(ChargeManagerTest, DimensionTierAndDimensionDistanceItem) {
  ChargeFixture fixture;
  ChargeRule rule;
  rule.charge_code_ind = 0;
  rule.secondary_ind = fixture.charge.add_secondary(0, "BASE");
  rule.condition_type = ChargeConditionType::DIMENSION;
  rule.condition_dim_ind = 0;
  rule.tier_begin = 0;
  rule.tier_len = 2;
  fixture.charge.rules.push_back(rule);
  fixture.charge.tiers.push_back(ChargeTier{0, 1000, 0, 1});
  fixture.charge.tiers.push_back(ChargeTier{1000, std::numeric_limits<long>::max(), 1, 2});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, 500000, -1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::DIMENSION, 5000, 0});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::DIMENSION_DISTANCE, 100, 0});
  fixture.build();

  // 12.50kg：0.5 * 12.5 = 6.25 元；0.01 * 12.5 * 10km = 1.25 元；合计 7.5 元
  const long dim_vals[] = {1250};
  const Location* const stops[] = {fixture.locations.get_location("SH")};
  EXPECT_EQ(fixture.charge.evaluate(fixture.make_view(10000, 1, 1, dim_vals, stops, 1)).amount,
            75000);
}

TEST(ChargeManagerTest, PathMismatchIsZero) {
  ChargeFixture fixture;
  ChargeRule rule;
  rule.charge_code_ind = 0;
  rule.secondary_ind = fixture.charge.add_secondary(0, "BASE");
  rule.path_type = ChargePathType::OD_PAIR;
  rule.path_label_ind4labelset = 0;
  rule.path_values = {fixture.zone_value("SH"), fixture.zone_value("SZ")};
  rule.tier_begin = 0;
  rule.tier_len = 1;
  fixture.charge.rules.push_back(rule);
  fixture.charge.tiers.push_back(ChargeTier{0, std::numeric_limits<long>::max(), 0, 1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, 1000000, -1});
  fixture.build();

  const Location* const stops[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("WX")};
  const Location* const picks[] = {fixture.locations.get_location("SH")};
  const Location* const drops[] = {fixture.locations.get_location("WX")};
  const ChargeResult result =
      fixture.charge.evaluate(fixture.make_view(0, 1, 1, nullptr, stops, 2, picks, drops, 1));
  EXPECT_FALSE(result.matched);
  EXPECT_EQ(result.amount, 0);
}

TEST(ChargeManagerTest, ConsecutiveDuplicateStopsAreCollapsed) {
  ChargeFixture fixture;
  ChargeRule rule;
  rule.charge_code_ind = 0;
  rule.secondary_ind = fixture.charge.add_secondary(0, "BASE");
  rule.path_type = ChargePathType::ORDERED_ROUTE;
  rule.path_label_ind4labelset = 0;
  rule.path_values = {fixture.zone_value("SH"), fixture.zone_value("SZ"), fixture.zone_value("WX")};
  rule.tier_begin = 0;
  rule.tier_len = 1;
  fixture.charge.rules.push_back(rule);
  fixture.charge.tiers.push_back(ChargeTier{0, std::numeric_limits<long>::max(), 0, 1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, 1000000, -1});
  fixture.build();

  // SH, SH, SZ, SZ, WX 合并为 SH, SZ, WX
  const Location* const stops[] = {
      fixture.locations.get_location("SH"), fixture.locations.get_location("SH"),
      fixture.locations.get_location("SZ"), fixture.locations.get_location("SZ"),
      fixture.locations.get_location("WX")};
  EXPECT_EQ(fixture.charge.evaluate(fixture.make_view(0, 2, 3, nullptr, stops, 5)).amount, 1000000);
}

TEST(ChargeManagerTest, OdPairUsesOrderLocationsNotRouteEndpoints) {
  ChargeFixture fixture;
  ChargeRule rule;
  rule.charge_code_ind = 0;
  rule.secondary_ind = fixture.charge.add_secondary(0, "BASE");
  rule.path_type = ChargePathType::OD_PAIR;
  rule.path_label_ind4labelset = 0;
  rule.path_values = {fixture.zone_value("SH"), fixture.zone_value("SZ")};
  rule.tier_begin = 0;
  rule.tier_len = 1;
  fixture.charge.rules.push_back(rule);
  fixture.charge.tiers.push_back(ChargeTier{0, std::numeric_limits<long>::max(), 0, 1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, 1000000, -1});
  fixture.build();

  // 线路首末都是 WX，但订单是 SH -> SZ，OD_PAIR 仍应命中
  const Location* const stops[] = {
      fixture.locations.get_location("WX"), fixture.locations.get_location("SH"),
      fixture.locations.get_location("SZ"), fixture.locations.get_location("WX")};
  const Location* const picks[] = {fixture.locations.get_location("SH")};
  const Location* const drops[] = {fixture.locations.get_location("SZ")};
  EXPECT_EQ(fixture.charge.evaluate(fixture.make_view(0, 2, 2, nullptr, stops, 4, picks, drops, 1))
                .amount,
            1000000);
}

TEST(ChargeManagerTest, OdPairRequiresEveryOdToBeQuoted) {
  ChargeFixture fixture;
  ChargeRule rule;
  rule.charge_code_ind = 0;
  rule.secondary_ind = fixture.charge.add_secondary(0, "BASE");
  rule.path_type = ChargePathType::OD_PAIR;
  rule.path_label_ind4labelset = 0;
  rule.path_values = {fixture.zone_value("SH"), fixture.zone_value("SZ")};
  rule.tier_begin = 0;
  rule.tier_len = 1;
  fixture.charge.rules.push_back(rule);
  fixture.charge.tiers.push_back(ChargeTier{0, std::numeric_limits<long>::max(), 0, 1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, 1000000, -1});
  fixture.build();

  const Location* const stops[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SZ"),
                                   fixture.locations.get_location("WX")};
  const Location* const picks[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SH")};
  // 混合 OD：SH-WX 没有报价，整个次级编码不命中
  const Location* const mixed_drops[] = {fixture.locations.get_location("SZ"),
                                         fixture.locations.get_location("WX")};
  EXPECT_EQ(
      fixture.charge.evaluate(fixture.make_view(0, 1, 2, nullptr, stops, 3, picks, mixed_drops, 2))
          .amount,
      0);
  // 全部订单都是 SH -> SZ：命中
  const Location* const same_drops[] = {fixture.locations.get_location("SZ"),
                                        fixture.locations.get_location("SZ")};
  EXPECT_EQ(
      fixture.charge.evaluate(fixture.make_view(0, 1, 2, nullptr, stops, 3, picks, same_drops, 2))
          .amount,
      1000000);
}

TEST(ChargeManagerTest, MultiOdRequiresCoverageAndTakesMaxQuote) {
  ChargeFixture fixture;
  add_od_rule(fixture, 0, "BASE", "SH", "SZ", 1000000);
  add_od_rule(fixture, 0, "BASE", "SH", "HZ", 1500000);
  fixture.build();

  const Location* const stops[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SZ"),
                                   fixture.locations.get_location("HZ")};
  const Location* const picks[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SH")};
  // SH-SZ + SH-HZ 都有报价：取最贵的 150 元
  const Location* const drops[] = {fixture.locations.get_location("SZ"),
                                   fixture.locations.get_location("HZ")};
  EXPECT_EQ(fixture.charge.evaluate(fixture.make_view(0, 1, 2, nullptr, stops, 3, picks, drops, 2))
                .amount,
            1500000);

  // 只有 SH-SZ：只按 SH-SZ 报价
  const Location* const one_drop[] = {fixture.locations.get_location("SZ")};
  EXPECT_EQ(
      fixture.charge.evaluate(fixture.make_view(0, 1, 2, nullptr, stops, 3, picks, one_drop, 1))
          .amount,
      1000000);

  // SH-SZ + SH-WX：SH-WX 没有报价，整个次级编码不可计费
  const Location* const mixed_drops[] = {fixture.locations.get_location("SZ"),
                                         fixture.locations.get_location("WX")};
  EXPECT_EQ(
      fixture.charge.evaluate(fixture.make_view(0, 1, 2, nullptr, stops, 3, picks, mixed_drops, 2))
          .amount,
      0);
}

TEST(ChargeManagerTest, MultiOdPartialSecondaryIsSkipped) {
  ChargeFixture fixture;
  // BASE 覆盖 SH-SZ + SH-HZ：取最贵的 150
  add_od_rule(fixture, 0, "BASE", "SH", "SZ", 1000000);
  add_od_rule(fixture, 0, "BASE", "SH", "HZ", 1500000);
  // FUEL 只覆盖 SH-SZ，缺少 SH-HZ 报价，整个 FUEL 次级编码不可用
  add_od_rule(fixture, 0, "FUEL", "SH", "SZ", 9000000);
  fixture.build();

  const Location* const stops[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SZ"),
                                   fixture.locations.get_location("HZ")};
  const Location* const picks[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SH")};
  const Location* const drops[] = {fixture.locations.get_location("SZ"),
                                   fixture.locations.get_location("HZ")};
  EXPECT_EQ(fixture.charge.evaluate(fixture.make_view(0, 1, 2, nullptr, stops, 3, picks, drops, 2))
                .amount,
            1500000);
}

TEST(ChargeManagerTest, OdTierGapMakesSecondaryIncomplete) {
  ChargeFixture fixture;

  // SH-SZ 的里程阶梯只有 [0, 10) km，当前 50 km 不匹配
  ChargeRule rule;
  rule.charge_code_ind = 0;
  rule.secondary_ind = fixture.charge.add_secondary(0, "BASE");
  rule.path_type = ChargePathType::OD_PAIR;
  rule.path_label_ind4labelset = 0;
  rule.path_values = {fixture.zone_value("SH"), fixture.zone_value("SZ")};
  rule.condition_type = ChargeConditionType::DISTANCE;
  rule.tier_begin = static_cast<int>(fixture.charge.tiers.size());
  rule.tier_len = 1;
  fixture.charge.rules.push_back(rule);
  fixture.charge.tiers.push_back(
      ChargeTier{0, 10000, static_cast<int>(fixture.charge.items.size()), 1});
  fixture.charge.items.push_back(ChargeItem{ChargeItemType::FLAT, 1000000, -1});

  // SH-HZ 正常报价
  add_od_rule(fixture, 0, "BASE", "SH", "HZ", 1500000);
  fixture.build();

  const Location* const stops[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SZ"),
                                   fixture.locations.get_location("HZ")};
  const Location* const picks[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SH")};
  const Location* const drops[] = {fixture.locations.get_location("SZ"),
                                   fixture.locations.get_location("HZ")};
  // SH-SZ 没有可用阶梯，整个 BASE 次级编码不可计费
  EXPECT_EQ(
      fixture.charge.evaluate(fixture.make_view(50000, 1, 2, nullptr, stops, 3, picks, drops, 2))
          .amount,
      0);
}

TEST(ChargeManagerTest, CsvReaderRejectsOverlappingTiers) {
  ChargeFixture fixture;
  ChargeCsvReader reader(YUN_CHARGE_OVERLAP_TEST_DATA_DIR, spdlog::default_logger(), &fixture.dims,
                         &fixture.labels, &fixture.carriers, &fixture.models, &fixture.locations);
  std::unique_ptr<ChargeManager> manager;
  EXPECT_THROW(manager.reset(reader.load()), std::runtime_error);
}

TEST(ChargeManagerTest, LazyLoadsOnlyRequiredFields) {
  ChargeFixture fixture;
  add_flat_rule(fixture, 0, "BASE", 1000000);
  fixture.build();

  const Location* const stops[] = {fixture.locations.get_location("SH")};
  auto view = fixture.make_view(0, 1, 1, nullptr, stops, 1);
  const ChargeResult result = fixture.charge.evaluate(view);
  EXPECT_TRUE(result.matched);
  EXPECT_EQ(result.amount, 1000000);

  // 纯 FLAT 规则只需要车辆索引
  EXPECT_EQ(view.vehicle_calls, 1);
  EXPECT_EQ(view.distance_calls, 0);
  EXPECT_EQ(view.pick_count_calls, 0);
  EXPECT_EQ(view.drop_count_calls, 0);
  EXPECT_EQ(view.dims_calls, 0);
  EXPECT_EQ(view.stops_calls, 0);
  EXPECT_EQ(view.stop_count_calls, 0);
  EXPECT_EQ(view.order_pick_calls, 0);
  EXPECT_EQ(view.order_drop_calls, 0);
  EXPECT_EQ(view.order_count_calls, 0);
}

TEST(ChargeManagerTest, DisabledManagerIsZero) {
  ChargeFixture fixture;
  fixture.build();
  const Location* const stops[] = {fixture.locations.get_location("SH")};
  const ChargeResult result =
      fixture.charge.evaluate(fixture.make_view(0, 1, 1, nullptr, stops, 1));
  EXPECT_FALSE(result.matched);
  EXPECT_EQ(result.amount, 0);
}

TEST(ChargeManagerTest, CsvReaderLoadsAndEvaluates) {
  ChargeFixture fixture;
  ChargeCsvReader reader(YUN_CHARGE_TEST_DATA_DIR, spdlog::default_logger(), &fixture.dims,
                         &fixture.labels, &fixture.carriers, &fixture.models, &fixture.locations);
  std::unique_ptr<ChargeManager> manager(reader.load());
  ASSERT_TRUE(manager->enabled);
  ASSERT_EQ(manager->rules.size(), 2U);

  const Location* const stops[] = {fixture.locations.get_location("SH"),
                                   fixture.locations.get_location("SZ")};
  const Location* const picks[] = {fixture.locations.get_location("SH")};
  const Location* const drops[] = {fixture.locations.get_location("SZ")};
  // 50 km：BASE = 100 + 2.35*50 = 217.5；FUEL = 0.3*50 = 15；TC1 = 217.5
  EXPECT_EQ(
      manager->evaluate(fixture.make_view(50000, 1, 1, nullptr, stops, 2, picks, drops, 1)).amount,
      2175000);
  // 120 km：BASE = 80 + 2*120 = 320；FUEL = 0.3*120 = 36；TC1 = 320
  EXPECT_EQ(
      manager->evaluate(fixture.make_view(120000, 1, 1, nullptr, stops, 2, picks, drops, 1)).amount,
      3200000);
}
