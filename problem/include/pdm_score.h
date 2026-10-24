/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cc_constraint_code.h"
#include "hc_constraint_code.h"
#include "pdm_infeasible.h"
#include "sc_constraint_code.h"

/**
 * @brief the score of hard constraint
 */
class HardConstrScore {
 public:
  using UPtr = std::unique_ptr<HardConstrScore>;
  using VecUPtr = std::vector<UPtr>;
  /**
   * @brief unique encoding with constraints
   */
  const HardConstrCode code;
  /**
   * @brief is feasible
   */
  const bool feasible;
  /**
   * @brief weight of the constraint
   * @details default weigh=1
   */
  const double weight;
  /**
   * @brief score of the constraint
   * @details score=0 is the feasible
   */
  const double value;
  /**
   * @brief is the constraint score related to the sequence
   */
  const bool seq_type;
  /**
   * @brief is the constraint score related to the vehicle
   */
  const bool vehicle_type;
  /**
   * @brief infeasible reason
   */
  const InfeasibleReason reason;

  explicit HardConstrScore(const HardConstrCode code, const bool feasible, const double weight,
                           const double value, const bool seq_type, const bool vehicle_type,
                           InfeasibleReason reason)
      : code(code),
        feasible(feasible),
        weight(weight),
        value(value),
        seq_type(seq_type),
        vehicle_type(vehicle_type),
        reason(std::move(reason)) {}

  explicit HardConstrScore(const HardConstrCode code)
      : code(code),
        feasible(true),
        weight(1),
        value(0),
        seq_type(false),
        vehicle_type(false),
        reason(InfeasibleReasonCollection::FEASIBLE) {}

  explicit HardConstrScore(const UPtr& score)
      : code(score->code),
        feasible(score->feasible),
        weight(score->weight),
        value(score->value),
        seq_type(score->seq_type),
        vehicle_type(score->vehicle_type),
        reason(score->reason) {};

  [[nodiscard]] double get_score() const { return this->weight * this->value; }

  [[nodiscard]] bool is_feasible() const { return this->feasible; }

  [[nodiscard]] bool is_infeasible() const { return !this->feasible; }

  [[nodiscard]] bool is_seq() const { return this->seq_type; }

  [[nodiscard]] bool is_vehicle() const { return this->vehicle_type; }
};

/**
 * @brief the score of soft constraint
 */
class SoftConstrScore {
 public:
  using UPtr = std::unique_ptr<SoftConstrScore>;
  using VecUPtr = std::vector<UPtr>;
  /**
   * @brief unique encoding with constraints
   */
  const SoftConstrCode code;
  /**
   * @brief weight of the constraint
   * @details default weigh=1
   */
  const double weight;
  /**
   * @brief score of the constraint
   * @details score=0 is the feasible
   */
  const double value;

  explicit SoftConstrScore(const SoftConstrCode code, const double weight, const double value)
      : code(code), weight(weight), value(value) {}

  explicit SoftConstrScore(const UPtr& score)
      : code(score->code), weight(score->weight), value(score->value) {}

  [[nodiscard]] double get_score() const { return this->weight * this->value; }
};

/**
 * @brief the score of cost constraint
 */
class CostConstrScore {
 public:
  using UPtr = std::unique_ptr<CostConstrScore>;
  using VecUPtr = std::vector<UPtr>;
  /**
   * @brief unique encoding with constraints
   */
  const CostConstrCode code;
  /**
   * @brief is feasible
   */
  const bool feasible;
  /**
   * @brief weight of the constraint
   * @details default weigh=1
   */
  const double weight;
  /**
   * @brief score of the constraint
   * @details score=0 is the feasible
   */
  const double value;
  /**
   * @brief is the constraint score related to the sequence
   */
  const bool seq_type;
  /**
   * @brief is the constraint score related to the vehicle
   */
  const bool vehicle_type;
  /**
   * @brief infeasible reason
   */
  const InfeasibleReason reason;

  CostConstrScore(const CostConstrCode code, const bool feasible, const double weight, const double value,
                  const bool seq_type, const bool vehicle_type, InfeasibleReason reason)
      : code(code),
        feasible(feasible),
        weight(weight),
        value(value),
        seq_type(seq_type),
        vehicle_type(vehicle_type),
        reason(std::move(reason)) {}

  CostConstrScore(const CostConstrCode code, const double weight, const double value, const bool seq_type,
                  const bool vehicle_type)
      : code(code),
        feasible(true),
        weight(weight),
        value(value),
        seq_type(seq_type),
        vehicle_type(vehicle_type),
        reason(InfeasibleReasonCollection::FEASIBLE) {}

  explicit CostConstrScore(const UPtr& score)
      : code(score->code),
        feasible(score->feasible),
        weight(score->weight),
        value(score->value),
        seq_type(score->seq_type),
        vehicle_type(score->vehicle_type),
        reason(score->reason) {}

  [[nodiscard]] double get_score() const { return this->weight * this->value; }

  [[nodiscard]] bool is_feasible() const { return this->feasible; }

  [[nodiscard]] bool is_infeasible() const { return !this->feasible; }

  [[nodiscard]] bool is_seq() const { return this->seq_type; }

  [[nodiscard]] bool is_vehicle() const { return this->vehicle_type; }
};
