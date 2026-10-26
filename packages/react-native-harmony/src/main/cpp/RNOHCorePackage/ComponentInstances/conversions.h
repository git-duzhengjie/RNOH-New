/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once
#include <arkui/native_type.h>
#include "react/renderer/components/scrollview/primitives.h"
#include "react/renderer/components/view/conversions.h"
#include "react/renderer/graphics/RectangleEdges.h"

namespace rnoh {
inline ArkUI_ScrollSnapAlign getArkUI_ScrollSnapAlign(
    facebook::react::ScrollViewSnapToAlignment snapToAlignment) {
  if (snapToAlignment == facebook::react::ScrollViewSnapToAlignment::Start) {
    return ArkUI_ScrollSnapAlign::ARKUI_SCROLL_SNAP_ALIGN_START;
  } else if (
      snapToAlignment == facebook::react::ScrollViewSnapToAlignment::Center) {
    return ArkUI_ScrollSnapAlign::ARKUI_SCROLL_SNAP_ALIGN_CENTER;
  } else if (
      snapToAlignment == facebook::react::ScrollViewSnapToAlignment::End) {
    return ArkUI_ScrollSnapAlign::ARKUI_SCROLL_SNAP_ALIGN_END;
  } else {
    return ArkUI_ScrollSnapAlign::ARKUI_SCROLL_SNAP_ALIGN_NONE;
  }
}

/**
 * @internal
 */
inline facebook::react::RectangleEdges<facebook::react::Float> resolveEdges(
    std::function<facebook::yoga::StyleLength(facebook::yoga::Edge)>
        getEdgeValue,
    bool isRTL = false) {
  auto left = getEdgeValue(facebook::yoga::Edge::Left);
  auto top = getEdgeValue(facebook::yoga::Edge::Top);
  auto right = getEdgeValue(facebook::yoga::Edge::Right);
  auto bottom = getEdgeValue(facebook::yoga::Edge::Bottom);
  auto start = getEdgeValue(facebook::yoga::Edge::Start);
  auto end = getEdgeValue(facebook::yoga::Edge::End);
  auto horizontal = getEdgeValue(facebook::yoga::Edge::Horizontal);
  auto vertical = getEdgeValue(facebook::yoga::Edge::Vertical);
  auto all = getEdgeValue(facebook::yoga::Edge::All);
  facebook::react::Float leftEdge = 0.0;
  facebook::react::Float topEdge = 0.0;
  facebook::react::Float bottomEdge = 0.0;
  facebook::react::Float rightEdge = 0.0;
  if (all.isDefined()) {
    leftEdge = topEdge = bottomEdge = rightEdge = all.value().unwrap();
  }
  if (horizontal.isDefined()) {
    leftEdge = rightEdge = horizontal.value().unwrap();
  }
  if (vertical.isDefined()) {
    topEdge = bottomEdge = vertical.value().unwrap();
  }
  if (isRTL) {
    leftEdge = end.value().unwrapOrDefault(leftEdge);
    rightEdge = start.value().unwrapOrDefault(rightEdge);
  } else {
    leftEdge = start.value().unwrapOrDefault(leftEdge);
    rightEdge = end.value().unwrapOrDefault(rightEdge);
  }
  leftEdge = left.value().unwrapOrDefault(leftEdge);
  topEdge = top.value().unwrapOrDefault(topEdge);
  rightEdge = right.value().unwrapOrDefault(rightEdge);
  bottomEdge = bottom.value().unwrapOrDefault(bottomEdge);
  return {leftEdge, topEdge, rightEdge, bottomEdge};
}

inline facebook::react::Point getEndPoint(const facebook::react::Rect& rect) {
  return facebook::react::Point{
      rect.origin.x + rect.size.width, rect.origin.y + rect.size.height};
}

inline bool rectIntersects(
    const facebook::react::Rect& rect1,
    const facebook::react::Rect& rect2) {
  return !(
      ((rect2.origin.x < rect1.origin.x &&
        getEndPoint(rect2).x < rect1.origin.x) ||
       (rect2.origin.x > getEndPoint(rect1).x &&
        getEndPoint(rect2).x > getEndPoint(rect1).x)) ||
      ((rect2.origin.y < rect1.origin.y &&
        getEndPoint(rect2).y < rect1.origin.y) ||
       (rect2.origin.y > getEndPoint(rect1).y &&
        getEndPoint(rect2).y > getEndPoint(rect1).y)));
}

} // namespace rnoh