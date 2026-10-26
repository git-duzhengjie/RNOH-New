/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <gmock/gmock.h>
#include <vector>
#include "RNOH/TouchTarget.h"
#include "react/renderer/components/view/TouchEventEmitter.h"

namespace rnoh {

/**
 * @brief gmock implementation of the TouchTarget abstract base class.
 *
 * computeChildPoint is intentionally NOT mocked: tests exercise the real
 * TouchTarget::computeChildPoint implementation (TouchTarget.cpp), which
 * calls the mocked accessors below plus the global `dispatcher` offset.
 * Configure behavior with ON_CALL/EXPECT_CALL.
 */
class MockTouchTarget : public TouchTarget {
 public:
  MOCK_METHOD(facebook::react::Point, getCurrentOffset, (), (const, override));
  MOCK_METHOD(bool, containsPoint, (const facebook::react::Point&), (const, override));
  MOCK_METHOD(bool, containsPointInBoundingBox, (const facebook::react::Point&), (override));
  MOCK_METHOD(bool, canHandleTouch, (), (const, override));
  MOCK_METHOD(bool, canChildrenHandleTouch, (), (const, override));
  MOCK_METHOD(bool, canSubtreeHandleTouch, (const facebook::react::Point&), (override));
  MOCK_METHOD(bool, isRootTouchTarget, (), (const, override));
  MOCK_METHOD(bool, isHandlingTouches, (), (const, override));
  MOCK_METHOD(bool, isJSResponder, (), (const, override));
  MOCK_METHOD(facebook::react::Tag, getTouchTargetTag, (), (const, override));
  MOCK_METHOD(facebook::react::SharedTouchEventEmitter, getTouchEventEmitter, (), (const, override));
  MOCK_METHOD(std::vector<Shared>, getTouchTargetChildren, (), (override));
  MOCK_METHOD(facebook::react::LayoutMetrics, getLayoutMetrics, (), (const, override));
  MOCK_METHOD(facebook::react::Transform, getTransform, (), (const, override));
  MOCK_METHOD(Shared, getTouchTargetParent, (), (const, override));
  MOCK_METHOD(facebook::react::Rect, getBoundingBox, (), (override));
  MOCK_METHOD(void, markBoundingBoxAsDirty, (), (override));
  MOCK_METHOD(bool, isClippingSubviews, (), (const, override));
};

} // namespace rnoh
