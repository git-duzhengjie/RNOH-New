/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "LeastSquareSolver.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cmath>
#include <limits>

using namespace rnoh;
using ::testing::DoubleNear;
using ::testing::ElementsAre;

constexpr double kEps = 1e-6;

class VectorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    a_ = Vector(3);
    a_.set(0, 1.0);
    a_.set(1, 2.0);
    a_.set(2, 3.0);
  }

  Vector a_{0};
};

TEST_F(VectorTest, Constructor_CreatesZeroVector) {
  Vector v(4);
  EXPECT_DOUBLE_EQ(v.get(0), 0.0);
  EXPECT_DOUBLE_EQ(v.get(1), 0.0);
  EXPECT_DOUBLE_EQ(v.get(2), 0.0);
  EXPECT_DOUBLE_EQ(v.get(3), 0.0);
}

TEST_F(VectorTest, GetSet_WorksCorrectly) {
  Vector v(2);
  v.set(0, 3.14);
  v.set(1, 2.72);
  EXPECT_DOUBLE_EQ(v.get(0), 3.14);
  EXPECT_DOUBLE_EQ(v.get(1), 2.72);
}

TEST_F(VectorTest, Dot_ComputesCorrectly) {
  Vector b(3);
  b.set(0, 4.0);
  b.set(1, 5.0);
  b.set(2, 6.0);
  EXPECT_DOUBLE_EQ(a_.dot(b), 32.0);
}

TEST_F(VectorTest, Dot_OrthogonalVectors_ReturnsZero) {
  Vector b(3);
  b.set(0, -2.0);
  b.set(1, 1.0);
  b.set(2, 0.0);
  EXPECT_DOUBLE_EQ(a_.dot(b), 0.0);
}

TEST_F(VectorTest, Norm_UnitVector_ReturnsOne) {
  Vector v(3);
  v.set(0, 1.0);
  v.set(1, 0.0);
  v.set(2, 0.0);
  EXPECT_NEAR(v.norm(), 1.0, kEps);
}

TEST_F(VectorTest, Norm_ComputesCorrectly) {
  EXPECT_NEAR(a_.norm(), std::sqrt(14.0), kEps);
}

TEST(Vector, FromVOL_WithOffsetAndLength_ReturnsCorrectView) {
  std::vector<double> data = {10.0, 20.0, 30.0, 40.0, 50.0};
  auto view = Vector::fromVOL(data, 1, 3);
  EXPECT_DOUBLE_EQ(view.get(0), 20.0);
  EXPECT_DOUBLE_EQ(view.get(1), 30.0);
  EXPECT_DOUBLE_EQ(view.get(2), 40.0);
}

class MatrixTest : public ::testing::Test {
 protected:
  void SetUp() override {
    m_ = Matrix(2, 3);
    m_.set(0, 0, 1.0);
    m_.set(0, 1, 2.0);
    m_.set(0, 2, 3.0);
    m_.set(1, 0, 4.0);
    m_.set(1, 1, 5.0);
    m_.set(1, 2, 6.0);
  }

  Matrix m_{0, 0};
};

TEST_F(MatrixTest, GetSet_WorksCorrectly) {
  EXPECT_DOUBLE_EQ(m_.get(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(m_.get(0, 2), 3.0);
  EXPECT_DOUBLE_EQ(m_.get(1, 0), 4.0);
  EXPECT_DOUBLE_EQ(m_.get(1, 2), 6.0);
}

TEST_F(MatrixTest, GetRow_ReturnsCorrectVector) {
  auto row0 = m_.getRow(0);
  EXPECT_DOUBLE_EQ(row0.get(0), 1.0);
  EXPECT_DOUBLE_EQ(row0.get(1), 2.0);
  EXPECT_DOUBLE_EQ(row0.get(2), 3.0);

  auto row1 = m_.getRow(1);
  EXPECT_DOUBLE_EQ(row1.get(0), 4.0);
  EXPECT_DOUBLE_EQ(row1.get(1), 5.0);
  EXPECT_DOUBLE_EQ(row1.get(2), 6.0);
}

TEST(PolynomialFit, Constructor_CreatesCorrectCoefficientCount) {
  PolynomialFit fit0(0);
  EXPECT_EQ(fit0.coefficients.size(), 1u);

  PolynomialFit fit1(1);
  EXPECT_EQ(fit1.coefficients.size(), 2u);

  PolynomialFit fit3(3);
  EXPECT_EQ(fit3.coefficients.size(), 4u);
}

class LeastSquareSolverTest : public ::testing::Test {
 protected:
  void SetUp() override {
    x_ = {1.0, 2.0, 3.0, 4.0, 5.0};
    w_ = {1.0, 1.0, 1.0, 1.0, 1.0};
  }

  std::vector<double> x_;
  std::vector<double> w_;
};

TEST_F(LeastSquareSolverTest, Solve_LinearFit_FitsCorrectly) {
  std::vector<double> y = {3.0, 5.0, 7.0, 9.0, 11.0};
  LeastSquareSolver solver(x_, y, w_);
  auto result = solver.solve(1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->coefficients.size(), 2u);
  EXPECT_NEAR(result->coefficients[0], 1.0, kEps);
  EXPECT_NEAR(result->coefficients[1], 2.0, kEps);
}

TEST_F(LeastSquareSolverTest, Solve_QuadraticFit_FitsCorrectly) {
  std::vector<double> y;
  for (auto xi : x_) {
    y.push_back(1.0 + 0.0 * xi + 1.0 * xi * xi);
  }
  LeastSquareSolver solver(x_, y, w_);
  auto result = solver.solve(2);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->coefficients.size(), 3u);
  EXPECT_NEAR(result->coefficients[0], 1.0, kEps);
  EXPECT_NEAR(result->coefficients[1], 0.0, kEps);
  EXPECT_NEAR(result->coefficients[2], 1.0, kEps);
}

TEST_F(LeastSquareSolverTest, Solve_ConstantFit_ReturnsCorrectCoefficients) {
  std::vector<double> y = {5.0, 5.0, 5.0, 5.0, 5.0};
  LeastSquareSolver solver(x_, y, w_);
  auto result = solver.solve(0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->coefficients.size(), 1u);
  EXPECT_NEAR(result->coefficients[0], 5.0, kEps);
}

TEST_F(LeastSquareSolverTest, Solve_DegreeExceedsDataSize_ReturnsNullopt) {
  std::vector<double> shortX = {1.0, 2.0};
  std::vector<double> shortY = {3.0, 5.0};
  std::vector<double> shortW = {1.0, 1.0};
  LeastSquareSolver solver(shortX, shortY, shortW);
  auto result = solver.solve(3);
  EXPECT_FALSE(result.has_value());
}

TEST_F(LeastSquareSolverTest, Solve_LinearlyDependentData_ReturnsNullopt) {
  std::vector<double> sameX = {1.0, 1.0, 1.0};
  std::vector<double> y = {2.0, 2.0, 2.0};
  std::vector<double> w = {1.0, 1.0, 1.0};
  LeastSquareSolver solver(sameX, y, w);
  auto result = solver.solve(1);
  EXPECT_FALSE(result.has_value());
}

TEST_F(LeastSquareSolverTest, Solve_WeightedFit_RespectsWeights) {
  std::vector<double> y = {3.0, 5.0, 7.0, 9.0, 100.0};
  std::vector<double> lowW = {1.0, 1.0, 1.0, 1.0, 0.001};
  LeastSquareSolver solver(x_, y, lowW);
  auto result = solver.solve(1);
  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(result->coefficients[0], 1.0, 0.1);
  EXPECT_NEAR(result->coefficients[1], 2.0, 0.1);
}

TEST_F(LeastSquareSolverTest, Solve_WithNaNInput_PropagatesNaNInCoefficients) {
  std::vector<double> nanY = {
      std::numeric_limits<double>::quiet_NaN(), 5.0, 7.0, 9.0, 11.0};
  LeastSquareSolver solver(x_, nanY, w_);
  auto result = solver.solve(1);
  ASSERT_TRUE(result.has_value());
  bool hasNaN = std::isnan(result->coefficients[0]) ||
      std::isnan(result->coefficients[1]);
  EXPECT_TRUE(hasNaN);
}

TEST_F(
    LeastSquareSolverTest,
    Solve_WithInfInput_PropagatesNonFiniteInCoefficients) {
  std::vector<double> infY = {
      std::numeric_limits<double>::infinity(), 5.0, 7.0, 9.0, 11.0};
  LeastSquareSolver solver(x_, infY, w_);
  auto result = solver.solve(1);
  ASSERT_TRUE(result.has_value());
  bool hasNonFinite = !std::isfinite(result->coefficients[0]) ||
      !std::isfinite(result->coefficients[1]);
  EXPECT_TRUE(hasNonFinite);
}

TEST_F(LeastSquareSolverTest, Solve_AllZeroWeights_ReturnsNullopt) {
  std::vector<double> zeroW = {0.0, 0.0, 0.0, 0.0, 0.0};
  std::vector<double> y = {3.0, 5.0, 7.0, 9.0, 11.0};
  LeastSquareSolver solver(x_, y, zeroW);
  auto result = solver.solve(1);
  EXPECT_FALSE(result.has_value());
}
