/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>

#include "dm_infeasible.h"

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
  const std::string code;
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

  HardConstrScore(const std::string& code, const bool feasible, const double weight,
                  const double value, const bool seq_type, const bool vehicle_type,
                  const InfeasibleReason reason)
      : code(code),
        feasible(feasible),
        weight(weight),
        value(value),
        seq_type(seq_type),
        vehicle_type(vehicle_type),
        reason(reason) {}

  HardConstrScore(const std::string& code)
      : code(code),
        feasible(true),
        weight(1),
        value(0),
        seq_type(false),
        vehicle_type(false),
        reason(InfeasibleReasonCollection::FEASIBLE) {}

  double get_score() const { return this->weight * this->value; }

  bool is_feasible() const { return this->feasible; }

  bool is_infeasible() const { return !this->feasible; }

  bool is_seq() const { return this->seq_type; }

  bool is_vehicle() const { return this->vehicle_type; }
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
  const std::string code;
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

  SoftConstrScore(const std::string& code, const double weight, const double value)
      : code(code), weight(weight), value(value) {}

  double get_score() const { return this->weight * this->value; }
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
  const std::string code;
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

  CostConstrScore(const std::string& code, const bool feasible, const double weight,
                  const double value, const bool seq_type, const bool vehicle_type,
                  const InfeasibleReason reason)
      : code(code),
        feasible(feasible),
        weight(weight),
        value(value),
        seq_type(seq_type),
        vehicle_type(vehicle_type),
        reason(reason) {}

  CostConstrScore(const std::string& code, const double weight, const double value,
                  const bool seq_type, const bool vehicle_type)
      : code(code),
        feasible(true),
        weight(weight),
        value(value),
        seq_type(seq_type),
        vehicle_type(vehicle_type),
        reason(InfeasibleReasonCollection::FEASIBLE) {}

  double get_score() const { return this->weight * this->value; }

  bool is_feasible() const { return this->feasible; }

  bool is_infeasible() const { return !this->feasible; }

  bool is_seq() const { return this->seq_type; }

  bool is_vehicle() const { return this->vehicle_type; }
};
