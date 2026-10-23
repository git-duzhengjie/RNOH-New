/**
 * RNOH-New: RNHGestureHandler 实现
 *
 * 手势状态机实现：tap / longPress / pan / swipe。
 * 与 RNHTouchHandler 配合使用，根据触摸序列识别手势并通过回调通知。
 */

#include "RNHGestureHandler.h"
#include "RNHComponentInstance.h"

#include <cmath>
#include <string>

namespace rnoh_new {

// ---------------------------------------------------------------------------
// 构造与析构
// ---------------------------------------------------------------------------

RNHGestureHandler::RNHGestureHandler() = default;

RNHGestureHandler::~RNHGestureHandler() = default;

// ---------------------------------------------------------------------------
// 依赖注入
// ---------------------------------------------------------------------------

void RNHGestureHandler::attachTo(
    std::shared_ptr<RNHComponentInstance> component) {
    component_ = std::move(component);
}

void RNHGestureHandler::setGestureCallback(GestureCallback callback) {
    callback_ = std::move(callback);
}

// ---------------------------------------------------------------------------
// 状态机入口
// ---------------------------------------------------------------------------

/**
 * 处理单个触摸事件，驱动手势状态机：
 * - Down：记录起始点、时间，初始化跟踪状态
 * - Move：计算位移；若超过 PAN_SLOP 则进入 pan 状态
 * - Up：根据总位移、持续时间、末速度判定 tap / pan / swipe
 * - Cancel：重置状态机
 */
void RNHGestureHandler::onTouchEvent(const RNHTouchEvent& event) {
    switch (event.action) {
        case RNHTouchAction::Down: {
            reset();
            isTracking_ = true;
            startX_ = event.x;
            startY_ = event.y;
            lastX_ = event.x;
            lastY_ = event.y;
            startTimeMs_ = event.timestamp;
            lastTimeMs_ = event.timestamp;
            break;
        }

        case RNHTouchAction::Move: {
            if (!isTracking_) {
                break;
            }

            float dxFromStart = event.x - startX_;
            float dyFromStart = event.y - startY_;
            float distance =
                std::sqrt(dxFromStart * dxFromStart + dyFromStart * dyFromStart);

            // 首次超过 PAN_SLOP，标记为 pan 开始
            if (!isPanning_ && distance > PAN_SLOP_PX) {
                isPanning_ = true;
                lastGesture_ = RNHGestureType::Pan;
                notifyGesture(RNHGestureType::Pan, dxFromStart, dyFromStart,
                              0.0f, 0.0f);
            }

            // 若处于 pan 状态，持续通知
            if (isPanning_) {
                float dx = event.x - lastX_;
                float dy = event.y - lastY_;
                notifyGesture(RNHGestureType::Pan, dx, dy, 0.0f, 0.0f);
            }

            // 长按识别：在 Down 后持续超过 500ms 且未 pan
            if (!isPanning_ && !longPressRecognized_ &&
                (event.timestamp - startTimeMs_) > LONG_PRESS_TIMEOUT_MS) {
                longPressRecognized_ = true;
                lastGesture_ = RNHGestureType::LongPress;
                notifyGesture(RNHGestureType::LongPress, 0.0f, 0.0f, 0.0f,
                              0.0f);
            }

            lastX_ = event.x;
            lastY_ = event.y;
            lastTimeMs_ = event.timestamp;
            break;
        }

        case RNHTouchAction::Up: {
            if (!isTracking_) {
                break;
            }

            float dx = event.x - startX_;
            float dy = event.y - startY_;
            float distance = std::sqrt(dx * dx + dy * dy);
            int64_t durationMs = event.timestamp - startTimeMs_;

            if (isPanning_) {
                // Pan 结束，计算末速度并判定 swipe
                int64_t dtMs = event.timestamp - lastTimeMs_;
                float velocityX = 0.0f;
                float velocityY = 0.0f;
                if (dtMs > 0) {
                    computeVelocity(event.x - lastX_, event.y - lastY_, dtMs,
                                    velocityX, velocityY);
                }

                float speed = std::sqrt(velocityX * velocityX +
                                        velocityY * velocityY);
                if (speed > SWIPE_VELOCITY_PX_S) {
                    RNHGestureType swipeType = RNHGestureType::None;
                    if (std::abs(velocityX) > std::abs(velocityY)) {
                        swipeType = velocityX > 0 ? RNHGestureType::SwipeRight
                                                  : RNHGestureType::SwipeLeft;
                    } else {
                        swipeType = velocityY > 0 ? RNHGestureType::SwipeDown
                                                  : RNHGestureType::SwipeUp;
                    }
                    lastGesture_ = swipeType;
                    notifyGesture(swipeType, dx, dy, velocityX, velocityY);
                } else {
                    // 仍是 pan
                    lastGesture_ = RNHGestureType::Pan;
                    notifyGesture(RNHGestureType::Pan, dx, dy, velocityX,
                                  velocityY);
                }
            } else if (distance < TAP_SLOP_PX &&
                       durationMs < LONG_PRESS_TIMEOUT_MS &&
                       !longPressRecognized_) {
                // 轻触
                lastGesture_ = RNHGestureType::Tap;
                notifyGesture(RNHGestureType::Tap, 0.0f, 0.0f, 0.0f, 0.0f);
            }

            // 本次手势序列结束，但保留 lastGesture_ 供 getLastRecognizedGesture 查询
            isTracking_ = false;
            isPanning_ = false;
            break;
        }

        case RNHTouchAction::Cancel: {
            reset();
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// 查询与回调
// ---------------------------------------------------------------------------

/**
 * 获取最近一次识别到的手势名称字符串
 */
std::string RNHGestureHandler::getLastRecognizedGesture() const {
    return gestureTypeToString(lastGesture_);
}

/**
 * 重置状态机
 */
void RNHGestureHandler::reset() {
    isTracking_ = false;
    isPanning_ = false;
    longPressRecognized_ = false;
    startX_ = 0.0f;
    startY_ = 0.0f;
    lastX_ = 0.0f;
    lastY_ = 0.0f;
    startTimeMs_ = 0;
    lastTimeMs_ = 0;
}

// ---------------------------------------------------------------------------
// 内部辅助
// ---------------------------------------------------------------------------

/**
 * 计算速度（px/s）
 */
void RNHGestureHandler::computeVelocity(float dx, float dy, int64_t dtMs,
                                        float& outVelocityX,
                                        float& outVelocityY) const {
    if (dtMs <= 0) {
        outVelocityX = 0.0f;
        outVelocityY = 0.0f;
        return;
    }
    float dtS = static_cast<float>(dtMs) / 1000.0f;
    outVelocityX = dx / dtS;
    outVelocityY = dy / dtS;
}

/**
 * 将手势类型转换为 React 事件名字符串
 */
std::string RNHGestureHandler::gestureTypeToString(RNHGestureType type) {
    switch (type) {
        case RNHGestureType::Tap:
            return "onTap";
        case RNHGestureType::LongPress:
            return "onLongPress";
        case RNHGestureType::Pan:
            return "onPan";
        case RNHGestureType::SwipeUp:
            return "onSwipeUp";
        case RNHGestureType::SwipeDown:
            return "onSwipeDown";
        case RNHGestureType::SwipeLeft:
            return "onSwipeLeft";
        case RNHGestureType::SwipeRight:
            return "onSwipeRight";
        default:
            return "";
    }
}

/**
 * 通知识别结果：更新 lastGesture_ 并触发回调
 */
void RNHGestureHandler::notifyGesture(RNHGestureType type, float dx, float dy,
                                      float velocityX, float velocityY) {
    lastGesture_ = type;
    if (callback_) {
        callback_(type, dx, dy, velocityX, velocityY);
    }
}

} // namespace rnoh_new
