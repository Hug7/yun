/**
 * Copyright (c) 2026 Qi Li
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include "bdm_dist_matrix.h"

static_assert(!std::is_copy_constructible_v<DistMatrix>);
static_assert(std::is_move_constructible_v<DistMatrix>);

TEST(DistMatrixTest, RejectsNonPositiveSize) {
  EXPECT_THROW((DistMatrix(0, 0)), std::invalid_argument);
  EXPECT_THROW((DistMatrix(0, -1)), std::invalid_argument);
}

TEST(DistMatrixTest, InitializesBoundaryRowsAndDiagonals) {
  const DistMatrix matrix(0, 3);

  EXPECT_EQ(matrix.get_dist(0, 1), 0);
  EXPECT_EQ(matrix.get_time(0, 1), 0);
  EXPECT_EQ(matrix.get_dist(1, 0), 0);
  EXPECT_EQ(matrix.get_dist(1, 1), 0);
  EXPECT_EQ(matrix.get_dist(2, 2), 0);

  EXPECT_EQ(matrix.get_dist(1, 2), DistMatrixParameter::MAX_DISTANCE);
  EXPECT_EQ(matrix.get_time(1, 2), DistMatrixParameter::MAX_TIME);
}

TEST(DistMatrixTest, SetsAndGetsDistTime) {
  DistMatrix matrix(0, 3);
  matrix.set_dist_time(1, 2, 123, 456);

  const DistTime value = matrix.get_dist_time(1, 2);
  EXPECT_EQ(value.dist, 123);
  EXPECT_EQ(value.time, 456);
  EXPECT_EQ(matrix.get_dist(1, 2), 123);
  EXPECT_EQ(matrix.get_time(1, 2), 456);
}

TEST(DistMatrixTest, RejectsOutOfRangeWrites) {
  DistMatrix matrix(0, 3);

  EXPECT_THROW(matrix.set_dist_time(-1, 0, 1, 1), std::out_of_range);
  EXPECT_THROW(matrix.set_dist_time(3, 0, 1, 1), std::out_of_range);
  EXPECT_THROW(matrix.set_dist_time(0, -1, 1, 1), std::out_of_range);
  EXPECT_THROW(matrix.set_dist_time(0, 3, 1, 1), std::out_of_range);
}

TEST(DistMatrixManagerTest, CreatesAndIndexesMatrix) {
  DistMatrixManager manager(4);
  manager.create_dist_matrix_code("A", "a");

  DistMatrixCode* code = manager.get_dist_matrix_code("A");
  ASSERT_NE(code, nullptr);
  DistMatrix* matrix = manager.get_dist_matrix(code);
  ASSERT_NE(matrix, nullptr);
  EXPECT_EQ(matrix->matrix_len, 4);

  manager.set_dist_time("A", 1, 2, 30, 40);
  EXPECT_EQ(matrix->get_dist(1, 2), 30);
  EXPECT_EQ(matrix->get_time(1, 2), 40);

  EXPECT_THROW(manager.set_dist_time("A", -1, 0, 1, 1), std::out_of_range);
  EXPECT_THROW(manager.set_dist_time("A", 0, 4, 1, 1), std::out_of_range);
}

TEST(DistMatrixManagerTest, RejectsInvalidCodeAndDuplicates) {
  DistMatrixManager manager(3);

  EXPECT_THROW(manager.get_dist_matrix(nullptr), std::invalid_argument);

  DistMatrixCode unknown("unknown", "unknown", 99);
  EXPECT_THROW(manager.get_dist_matrix(&unknown), std::out_of_range);

  EXPECT_THROW(manager.get_dist_matrix_code("missing"), std::runtime_error);

  manager.create_dist_matrix_code("A", "a");
  EXPECT_THROW(manager.create_dist_matrix_code("A", "a"), std::runtime_error);
  EXPECT_THROW(manager.set_dist_time("missing", 0, 0, 1, 1), std::runtime_error);
}
