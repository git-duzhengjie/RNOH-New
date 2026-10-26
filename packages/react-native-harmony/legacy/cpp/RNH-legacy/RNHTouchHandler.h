/**
 * RNOH-New: Touch Handler
 *
 * 处理来自 ArkUI 的原始触摸事件，完成命中测试、手势协调、
 * 事件分发（onPress/onPressIn/onPressOut/onLongPress）等逻辑。
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>

#include "RNHTouchEvent.h"

namespace rnoh_new {

// Forward declarations
class RNHComponentInstance;
class RNHEventDispatcher;
class RNHGestureHandler;

/**
 * 触摸事件处理器
 *
 * 职责：
 * 1. 接收原始触摸事件
 * 2. 通过命中测试找到目标组件
 * 3. 协调手势处理器（GestureHandler）识别 tap/longPress/pan
 * 4. 向目标组件派发 React 事件
 */
class RNHTouchHandler {
public:
    RNHTouchHandler();
    ~RNHTouchHandler();

    // 禁止拷贝，允许移动
    RNHTouchHandler(const RNHTouchHandler&) = delete;
    RNHTouchHandler& operator=(const RNHTouchHandler&) = delete;
    RNHTouchHandler(RNHTouchHandler&&) = default;
    RNHTouchHandler& operator=(RNHTouchHandler&&) = default;

    /**
     * 设置根组件（命中测试起点）
     */
    void setRootComponent(std::shared_ptr<RNHComponentInstance> root);

    /**
     * 设置事件分发器（用于向 JS 侧派发事件）
     */
    void setEventDispatcher(
        std::shared_ptr<RNHEventDispatcher> eventDispatcher);

    /**
     * 触摸事件入口（ArkUI 触摸回调会调用此方法）
     * @param surfaceId  Surface ID
     * @param action     动作：0=Down, 1=Up, 2=Move, 3=Cancel
     * @param x          触摸点 X 坐标（相对 root）
     * @param y          触摸点 Y 坐标（相对 root）
     * @param timestamp  事件时间戳（毫秒）
     */
    void onTouchEvent(int surfaceId, int action, float x, float y,
                      int64_t timestamp);

    /**
     * 绑定手势处理器（可选）
     */
    void attachGestureHandler(
        std::shared_ptr<RNHGestureHandler> gestureHandler);

private:
    /**
     * 命中测试：从根组件开始递归查找包含坐标的最深组件
     * @return 命中最深组件（可能为 nullptr）
     */
    std::shared_ptr<RNHComponentInstance> hitTest(
        std::shared_ptr<RNHComponentInstance> root, float x, float y);

    /**
     * 派发事件到目标组件，支持冒泡
     */
    void dispatchEventToTarget(
        const std::shared_ptr<RNHComponentInstance>& target,
        const std::string& eventName, const std::string& payload);

    /**
     * 开始长按定时器（500ms 阈值）
     */
    void startLongPressTimer();

    /**
     * 停止长按定时器
     */
    void stopLongPressTimer();

    /**
     * 长按定时器回调
     */
    void onLongPressTimeout();

    // 当前触摸状态
    bool isPressed_ = false;
    bool longPressFired_ = false;
    float startX_ = 0.0f;
    float startY_ = 0.0f;
    float currentX_ = 0.0f;
    float currentY_ = 0.0f;
    int64_t startTimestamp_ = 0;
    int currentSurfaceId_ = -1;
    std::weak_ptr<RNHComponentInstance> currentTarget_;

    // 依赖
    std::weak_ptr<RNHComponentInstance> rootComponent_;
    std::weak_ptr<RNHEventDispatcher> eventDispatcher_;
    std::shared_ptr<RNHGestureHandler> gestureHandler_;

    // 长按定时器
    std::atomic<bool> longPressPending_{false};
    std::thread longPressThread_;
    std::mutex stateMutex_;

    // 常量
    static constexpr int64_t LONG_PRESS_TIMEOUT_MS = 500;
};

} // namespace rnoh_new
