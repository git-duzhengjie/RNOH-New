/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <gtest/gtest.h>
#include "MockArkUIAPI.h"
#include "RNOH/arkui/NativeNodeApi.h"
#include "RNOH/arkui/ScrollNode.h"

using namespace rnoh;

class ScrollNodeLegacyApiTest : public ::testing::Test {
 protected:
  void SetUp() override {
    resetMockState();
    NativeNodeApi::INSTANCE = nullptr;
  }
};

TEST_F(
    ScrollNodeLegacyApiTest,
    SetFlingSpeedLimit_BelowApi18_DoesNotSetAttribute) {
  ScrollNode node;

  auto& result = node.setFlingSpeedLimit(4200.5f);

  EXPECT_EQ(&result, &node);
  EXPECT_EQ(
      mockGetAttribute(
          node.getArkUINodeHandle(), NODE_SCROLL_FLING_SPEED_LIMIT),
      nullptr);
}

TEST_F(ScrollNodeLegacyApiTest, SetBackToTop_BelowApi15_DoesNotSetAttribute) {
  ScrollNode node;

  auto& result = node.setBackToTop(true);

  EXPECT_EQ(&result, &node);
  EXPECT_EQ(
      mockGetAttribute(node.getArkUINodeHandle(), NODE_SCROLL_BACK_TO_TOP),
      nullptr);
}

TEST_F(ScrollNodeLegacyApiTest, SetFadingEdge_BelowApi14_DoesNotSetAttribute) {
  ScrollNode node;

  auto& result = node.setFadingEdge(18.5f);

  EXPECT_EQ(&result, &node);
  EXPECT_EQ(
      mockGetAttribute(node.getArkUINodeHandle(), NODE_SCROLL_FADING_EDGE),
      nullptr);
}
