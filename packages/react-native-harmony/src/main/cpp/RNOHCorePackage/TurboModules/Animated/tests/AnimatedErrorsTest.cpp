/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "AnimatedErrors.h"
#include <gtest/gtest.h>

#include <string>

using namespace rnoh;

TEST(AnimatedNodeNotFoundError, Constructor_TagZero_MessageContainsZero) {
  facebook::react::Tag tag = 0;
  AnimatedNodeNotFoundError error(tag);
  EXPECT_EQ(
      std::string(error.what()), "Animated node with tag 0 does not exist");
}

TEST(AnimatedNodeNotFoundError, Constructor_PositiveTag_MessageContainsTag) {
  facebook::react::Tag tag = 42;
  AnimatedNodeNotFoundError error(tag);
  EXPECT_EQ(
      std::string(error.what()), "Animated node with tag 42 does not exist");
}

TEST(AnimatedNodeNotFoundError, Constructor_NegativeTag_MessageContainsTag) {
  facebook::react::Tag tag = -1;
  AnimatedNodeNotFoundError error(tag);
  EXPECT_EQ(
      std::string(error.what()), "Animated node with tag -1 does not exist");
}

TEST(AnimatedNodeNotFoundError, Constructor_LargeTag_MessageContainsTag) {
  facebook::react::Tag tag = 2147483647; // INT32_MAX
  AnimatedNodeNotFoundError error(tag);
  EXPECT_EQ(
      std::string(error.what()),
      "Animated node with tag 2147483647 does not exist");
}

TEST(AnimatedNodeNotFoundError, Constructor_InheritsRuntimeError) {
  AnimatedNodeNotFoundError error(1);
  EXPECT_TRUE(dynamic_cast<std::runtime_error*>(&error) != nullptr);
}

TEST(
    AnimatedNodeNotFoundError,
    Constructor_DifferentTags_ProduceDifferentMessages) {
  AnimatedNodeNotFoundError errorA(10);
  AnimatedNodeNotFoundError errorB(20);
  EXPECT_NE(std::string(errorA.what()), std::string(errorB.what()));
}
