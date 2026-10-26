/**
 * RNOH-New: RNHTouchHandler 实现
 *
 * 将 ArkUI 原始触摸事件转换为 React 触摸事件，
 * 完成命中测试、长按定时器、以及事件冒泡分发。
 */

#include "RNHTouchHandler.h"
#include "RNHComponentInstance.h"
#include "RNHEventDispatcher.h"
#include "RNHGestureHandler.h"

#include <cmath>
#include <thread>
#include <chrono>

namespace rnoh_new {

// 静态常量：允许的手指移动阈值（超过则取消长按/点击）
static constexpr float TOUCH_SLOP_PX = 10.0f;

// ---------------------------------------------------------------------------
// 构造与析构
// ---------------------------------------------------------------------------

RNHTouchHandler::RNHTouchHandler() = default;

RNHTouchHandler::~RNHTouchHandler() {
    stopLongPressTimer();
}

// ---------------------------------------------------------------------------
// 依赖注入
// ---------------------------------------------------------------------------

void RNHTouchHandler::setRootComponent(
    std::shared_ptr<RNHComponentInstance> root) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    rootComponent_ = root;
}

void RNHTouchHandler::setEventDispatcher(
    std::shared_ptr<RNHEventDispatcher> eventDispatcher) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    eventDispatcher_ = eventDispatcher;
}

void RNHTouchHandler::attachGestureHandler(
    std::shared_ptr<RNHGestureHandler> gestureHandler) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    gestureHandler_ = std::move(gestureHandler);
}

// ---------------------------------------------------------------------------
// 触摸事件入口
// ---------------------------------------------------------------------------

/**
 * 处理原始触摸事件：
 * 1. Down：命中测试 → 记录按压状态 → 启动长按定时器 → 派发 onPressIn
 * 2. Move：更新坐标 → 检查是否滑出阈值 → 通知手势处理器
 * 3. Up：停止长按定时器 → 派发 onPressOut/onPress → 若有手势结果则派发手势事件
 * 4. Cancel：停止长按定时器 → 派发 onPressOut
 */
void RNHTouchHandler::onTouchEvent(int surfaceId, int action, float x, float y,
                                   int64_t timestamp) {
    std::lock_guard<std::mutex> lock(stateMutex_);

    RNHTouchEvent event;
    event.surfaceId = surfaceId;
    event.action = static_cast<RNHTouchAction>(action);
    event.x = x;
    event.y = y;
    event.timestamp = timestamp;

    switch (event.action) {
        case RNHTouchAction::Down: {
            isPressed_ = true;
            longPressFired_ = false;
            startX_ = x;
            startY_ = y;
            currentX_ = x;
            currentY_ = y;
            startTimestamp_ = timestamp;
            currentSurfaceId_ = surfaceId;

            // 命中测试
            auto root = rootComponent_.lock();
            auto target = root ? hitTest(root, x, y) : nullptr;
            currentTarget_ = target;

            // 启动长按定时器
            startLongPressTimer();

            // 向目标组件派发 onPressIn
            if (target) {
                std::string payload = "{\"x\":" + std::to_string(x) +
                                      ",\"y\":" + std::to_string(y) + "}";
                dispatchEventToTarget(target, "onPressIn", payload);

                // 同步通知手势处理器（Down）
                if (gestureHandler_) {
                    gestureHandler_->onTouchEvent(event);
                }
            }
            break;
        }

        case RNHTouchAction::Move: {
            currentX_ = x;
            currentY_ = y;

            // 若移动超过阈值且长按尚未触发，则取消长按
            float dx = x - startX_;
            float dy = y - startY_;
            if (std::sqrt(dx * dx + dy * dy) > TOUCH_SLOP_PX) {
                stopLongPressTimer();
            }

            // 同步通知手势处理器（Move）
            if (gestureHandler_) {
                gestureHandler_->onTouchEvent(event);
            }
            break;
        }

        case RNHTouchAction::Up: {
            stopLongPressTimer();

            auto target = currentTarget_.lock();
            if (target && isPressed_) {
                std::string payload = "{\"x\":" + std::to_string(x) +
                                      ",\"y\":" + std::to_string(y) +
                                      ",\"longPress\":" +
                                      (longPressFired_ ? "true" : "false") +
                                      "}";

                // 派发 onPressOut
                dispatchEventToTarget(target, "onPressOut", payload);

                // 若未触发长按，则派发 onPress
                if (!longPressFired_) {
                    dispatchEventToTarget(target, "onPress", payload);
                }

                // 通知手势处理器（Up），由其决定 tap/longPress/pan/swipe
                if (gestureHandler_) {
                    gestureHandler_->onTouchEvent(event);
                    auto gesture = gestureHandler_->getLastRecognizedGesture();
                    if (!gesture.empty()) {
                        dispatchEventToTarget(target, gesture,
                                              "{\"x\":" + std::to_string(x) +
                                                  ",\"y\":" +
                                                  std::to_string(y) + "}");
                    }
                }
            }

            isPressed_ = false;
            currentTarget_.reset();
            break;
        }

        case RNHTouchAction::Cancel: {
            stopLongPressTimer();

            auto target = currentTarget_.lock();
            if (target && isPressed_) {
                std::string payload = "{\"x\":" + std::to_string(x) +
                                      ",\"y\":" + std::to_string(y) + "}";
                dispatchEventToTarget(target, "onPressOut", payload);
            }

            isPressed_ = false;
            currentTarget_.reset();
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// 命中测试
// ---------------------------------------------------------------------------

/**
 * 从根组件开始递归查找包含坐标 (x, y) 的最深组件。
 * 策略：前序遍历，优先返回最深层包含该点的子组件。
 * 当前实现依赖组件的 layout 信息（x, y, width, height），
 * TODO: 后续接入 LayoutEngine 的真实布局结果。
 */
std::shared_ptr<RNHComponentInstance> RNHTouchHandler::hitTest(
    std::shared_ptr<RNHComponentInstance> root, float x, float y) {
    if (!root) {
        return nullptr;
    }

    // TODO: 通过 LayoutEngine 或 ArkUINode 获取组件当前布局矩形
    // 当前为占位实现：假设根组件范围内命中
    bool contains = (x >= 0 && y >= 0);
    if (!contains) {
        return nullptr;
    }

    // 获取子组件（通过 ArkUI 节点或组件内部维护的子组件列表）
    // TODO: RNHComponentInstance 当前未暴露子组件列表，需要扩展接口
    // 当前简化为直接返回 root，后续改为递归子节点命中测试
    return root;
}

// ---------------------------------------------------------------------------
// 事件派发（支持冒泡）
// ---------------------------------------------------------------------------

/**
 * 向目标组件派发事件，并沿父组件链向上冒泡。
 * 当前 RNHComponentInstance 未暴露父组件指针，因此只做单层分发。
 * TODO: 后续在 RNHComponentInstance 中加入 parent 指针实现完整冒泡。
 */
void RNHTouchHandler::dispatchEventToTarget(
    const std::shared_ptr<RNHComponentInstance>& target,
    const std::string& eventName, const std::string& payload) {
    if (!target) {
        return;
    }

    // 先调用组件自身的 dispatchEvent（可能直接发给 JS）
    target->dispatchEvent(eventName, payload);

    // 再尝试通过事件分发器转发
    auto dispatcher = eventDispatcher_.lock();
    if (dispatcher) {
        dispatcher->dispatchCustomEvent(currentSurfaceId_, target->getTag(),
                                        eventName, payload);
    }

    // TODO: 沿父组件链冒泡（需要 RNHComponentInstance::getParent）
}

// ---------------------------------------------------------------------------
// 长按定时器
// ---------------------------------------------------------------------------

/**
 * 启动一个后台线程，500ms 后触发长按回调。
 * 若线程已存在，先停止再重新启动。
 */
void RNHTouchHandler::startLongPressTimer() {
    stopLongPressTimer();

    longPressPending_ = true;
    auto self = std::weak_ptr<RNHTouchHandler>(
        std::shared_ptr<RNHTouchHandler>(this, [](RNHTouchHandler*) {}));

    longPressThread_ = std::thread([self]() {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(LONG_PRESS_TIMEOUT_MS));

        auto handler = self.lock();
        if (!handler) {
            return;
        }
        handler->onLongPressTimeout();
    });
}

/**
 * 停止长按定时器线程
 */
void RNHTouchHandler::stopLongPressTimer() {
    longPressPending_ = false;
    if (longPressThread_.joinable()) {
        // TODO: 使用条件变量实现可中断等待，避免阻塞 500ms
        longPressThread_.detach();
    }
}

/**
 * 长按定时器回调：若仍在按压状态，则派发 onLongPress
 */
void RNHTouchHandler::onLongPressTimeout() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (!longPressPending_ || !isPressed_) {
        return;
    }

    longPressFired_ = true;
    auto target = currentTarget_.lock();
    if (target) {
        std::string payload = "{\"x\":" + std::to_string(currentX_) +
                              ",\"y\":" + std::to_string(currentY_) + "}";
        dispatchEventToTarget(target, "onLongPress", payload);
    }
}

} // namespace rnoh_new
