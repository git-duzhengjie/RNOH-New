/**
 * RNOH-New: Gesture Handler
 *
 * 手势识别器：根据触摸事件序列识别 tap、longPress、pan、swipe。
 * 可与 RNHTouchHandler 协同工作，将识别结果通知调用方。
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <vector>

#include "RNHTouchEvent.h"

namespace rnoh_new {

// Forward declarations
class RNHComponentInstance;
class RNHTouchHandler;

/**
 * 手势类型
 */
enum class RNHGestureType {
    None,
    Tap,       // 轻触
    LongPress, // 长按
    Pan,       // 拖拽
    SwipeUp,   // 向上滑动
    SwipeDown, // 向下滑动
    SwipeLeft, // 向左滑动
    SwipeRight // 向右滑动
};

/**
 * 手势处理器
 *
 * 状态机规则：
 * - tap：Down → Up（期间位移 < TAP_SLOP，时间 < LONG_PRESS 阈值）
 * - longPress：Down 持续超过 500ms，期间位移 < TAP_SLOP
 * - pan：Down → Move 位移 > PAN_SLOP，持续跟踪直到 Up/Cancel
 * - swipe：pan 结束时，末速度 > SWIPE_VELOCITY，根据方向判定 SwipeXxx
 */
class RNHGestureHandler {
public:
    RNHGestureHandler();
    ~RNHGestureHandler();

    // 禁止拷贝，允许移动
    RNHGestureHandler(const RNHGestureHandler&) = delete;
    RNHGestureHandler& operator=(const RNHGestureHandler&) = delete;
    RNHGestureHandler(RNHGestureHandler&&) = default;
    RNHGestureHandler& operator=(RNHGestureHandler&&) = default;

    /**
     * 绑定到目标组件（可选，用于获取组件上下文）
     */
    void attachTo(std::shared_ptr<RNHComponentInstance> component);

    /**
     * 接收触摸事件，更新手势状态机
     */
    void onTouchEvent(const RNHTouchEvent& event);

    /**
     * 获取最近一次识别到的手势类型（调用后不清空）
     */
    std::string getLastRecognizedGesture() const;

    /**
     * 设置识别结果回调
     */
    using GestureCallback = std::function<void(RNHGestureType, float dx,
                                                float dy, float velocityX,
                                                float velocityY)>;
    void setGestureCallback(GestureCallback callback);

    /**
     * 重置状态机
     */
    void reset();

private:
    /**
     * 根据两次 Move 点计算速度（px/s）
     */
    void computeVelocity(float dx, float dy, int64_t dtMs,
                         float& outVelocityX, float& outVelocityY) const;

    /**
     * 将 RNHGestureType 转为事件名字符串
     */
    static std::string gestureTypeToString(RNHGestureType type);

    /**
     * 通知监听器并更新最后识别手势
     */
    void notifyGesture(RNHGestureType type, float dx, float dy,
                       float velocityX, float velocityY);

    // 目标组件
    std::weak_ptr<RNHComponentInstance> component_;

    // 识别结果回调
    GestureCallback callback_;

    // 当前状态
    bool isTracking_ = false;
    bool isPanning_ = false;
    bool longPressRecognized_ = false;
    RNHGestureType lastGesture_ = RNHGestureType::None;

    // 起始点/上一个点/当前点
    float startX_ = 0.0f;
    float startY_ = 0.0f;
    float lastX_ = 0.0f;
    float lastY_ = 0.0f;
    int64_t startTimeMs_ = 0;
    int64_t lastTimeMs_ = 0;

    // 常量阈值
    static constexpr float TAP_SLOP_PX = 10.0f;
    static constexpr float PAN_SLOP_PX = 10.0f;
    static constexpr float SWIPE_VELOCITY_PX_S = 500.0f;
    static constexpr int64_t LONG_PRESS_TIMEOUT_MS = 500;
};

} // namespace rnoh_new
