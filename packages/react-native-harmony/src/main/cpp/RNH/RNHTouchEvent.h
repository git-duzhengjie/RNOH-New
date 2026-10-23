/**
 * RNOH-New: Shared touch event definitions
 *
 * 供 RNHTouchHandler 与 RNHGestureHandler 共享的触摸事件类型。
 */

#pragma once

namespace rnoh_new {

/**
 * 触摸动作类型（与 Android/iOS 一致）
 */
enum class RNHTouchAction {
    Down = 0,
    Up = 1,
    Move = 2,
    Cancel = 3,
};

/**
 * 触摸事件上下文
 */
struct RNHTouchEvent {
    int surfaceId = -1;
    RNHTouchAction action = RNHTouchAction::Down;
    float x = 0.0f;
    float y = 0.0f;
    int64_t timestamp = 0;
};

} // namespace rnoh_new
