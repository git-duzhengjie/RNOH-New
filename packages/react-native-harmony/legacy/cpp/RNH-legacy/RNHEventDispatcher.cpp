/**
 * RNOH-New: RNHEventDispatcher 实现
 *
 * 将原生侧（ArkUI）产生的事件分发到 JS 侧（React）。
 * 支持三种优先级：Synchronous（立即）/ Asynchronous（入队）/ Deferred（缓存）。
 * 同时维护本地监听器表，允许 C++ 层订阅事件。
 * 所有操作加锁保证线程安全。
 */

#include "RNHEventDispatcher.h"

#include <utility>

namespace rnoh_new {

// ---------------------------------------------------------------------------
// 构造与析构
// ---------------------------------------------------------------------------

RNHEventDispatcher::RNHEventDispatcher() {
    // 构造函数：无需额外初始化，队列与监听器表均为空
}

RNHEventDispatcher::~RNHEventDispatcher() {
    // 析构函数：清空事件队列与监听器表
    std::lock_guard<std::mutex> lock(queueMutex_);
    eventQueue_.clear();
}

// ---------------------------------------------------------------------------
// 事件分发（统一入口）
// ---------------------------------------------------------------------------

/**
 * 按优先级分发事件：
 * - Synchronous：立即执行（先通知本地监听器，再调用 JS 回调）
 * - Asynchronous：加入队列，等待 processEventQueue 批量处理
 * - Deferred：同样入队，但标记为低优先级（JS 空闲时处理）
 */
void RNHEventDispatcher::dispatchEvent(const RNHEvent& event) {
    switch (event.priority) {
        case RNHEventPriority::Synchronous: {
            // 立即分发：先通知本地监听器
            notifyLocalListeners(event);

            // 再直接调用 JS 回调
            JSEventCallback callback;
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                callback = jsCallback_;
            }
            if (callback) {
                // TODO: 确保在 JS 线程执行（当前简化为同步调用）
                callback(event.eventName, event.payload);
            }
            break;
        }

        case RNHEventPriority::Asynchronous:
        case RNHEventPriority::Deferred: {
            // 入队等待批量处理
            std::lock_guard<std::mutex> lock(queueMutex_);
            eventQueue_.push_back(event);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// 批量处理队列
// ---------------------------------------------------------------------------

/**
 * 批量处理事件队列：
 * 1. 将队列整体换出（最小化持锁时间）
 * 2. 逐条通知本地监听器
 * 3. 逐条调用 JS 回调
 */
void RNHEventDispatcher::processEventQueue() {
    // 换出待处理队列
    std::vector<RNHEvent> pending;
    JSEventCallback callback;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        pending.swap(eventQueue_);
        callback = jsCallback_;
    }

    // 逐条处理
    for (const auto& event : pending) {
        // 通知本地监听器
        notifyLocalListeners(event);

        // 调用 JS 回调
        if (callback) {
            // TODO: 通过 JSI 转发到 JS 线程（当前简化为同步调用）
            callback(event.eventName, event.payload);
        }
    }
}

// ---------------------------------------------------------------------------
// 触摸/自定义事件便捷方法
// ---------------------------------------------------------------------------

/**
 * 分发触摸事件：构造带 JSON payload 的 RNHEvent 后走统一分发
 */
void RNHEventDispatcher::dispatchTouchEvent(int surfaceId, int targetTag,
                                            int action, float x, float y,
                                            int64_t timestamp) {
    RNHEvent event;
    event.eventName = "onTouch";
    event.surfaceId = surfaceId;
    event.targetTag = targetTag;
    event.priority = RNHEventPriority::Asynchronous;
    event.timestamp = timestamp;

    // 构造 JSON 载荷
    event.payload = "{\"action\":" + std::to_string(action) +
                    ",\"x\":" + std::to_string(x) +
                    ",\"y\":" + std::to_string(y) +
                    ",\"timestamp\":" + std::to_string(timestamp) + "}";

    dispatchEvent(std::move(event));
}

/**
 * 分发自定义事件：构造 RNHEvent 后走统一分发
 */
void RNHEventDispatcher::dispatchCustomEvent(int surfaceId, int targetTag,
                                             const std::string& eventName,
                                             const std::string& payload) {
    RNHEvent event;
    event.eventName = eventName;
    event.payload = payload;
    event.surfaceId = surfaceId;
    event.targetTag = targetTag;
    event.priority = RNHEventPriority::Asynchronous;
    // TODO: 使用真实时间戳（当前为 0 占位）
    event.timestamp = 0;

    dispatchEvent(std::move(event));
}

// ---------------------------------------------------------------------------
// 本地监听器管理
// ---------------------------------------------------------------------------

/**
 * 注册本地事件监听器（可重入，同一事件名可注册多个监听器）
 */
void RNHEventDispatcher::addEventListener(const std::string& eventName,
                                          EventListener listener) {
    std::lock_guard<std::mutex> lock(listenerMutex_);
    listeners_.insert(std::make_pair(eventName, std::move(listener)));
}

/**
 * 移除本地事件监听器
 * std::function 无法直接比较，通过 target<void(const RNHEvent&)>() 比较
 * 可调用对象的地址来匹配（仅对普通函数/捕获为空的 lambda 有效）
 */
void RNHEventDispatcher::removeEventListener(const std::string& eventName,
                                             EventListener listener) {
    std::lock_guard<std::mutex> lock(listenerMutex_);

    auto range = listeners_.equal_range(eventName);
    for (auto it = range.first; it != range.second;) {
        // 比较 std::function 内部持有的可调用对象地址
        if (it->second.target<void(const RNHEvent&)>() ==
            listener.target<void(const RNHEvent&)>()) {
            it = listeners_.erase(it);
        } else {
            ++it;
        }
    }
}

// ---------------------------------------------------------------------------
// JS 回调设置
// ---------------------------------------------------------------------------

/**
 * 设置 JS 事件回调（事件需要送达 JS 侧时调用）
 */
void RNHEventDispatcher::setJSEventCallback(JSEventCallback callback) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    jsCallback_ = std::move(callback);
}

// ---------------------------------------------------------------------------
// 内部方法：通知本地监听器
// ---------------------------------------------------------------------------

/**
 * 通知所有订阅了该事件名的本地监听器
 * 注意：监听器回调中可能再次调用 add/removeEventListener，
 * 因此先拷贝出监听器列表再逐个调用，避免死锁。
 */
void RNHEventDispatcher::notifyLocalListeners(const RNHEvent& event) {
    // 拷贝监听器列表（最小化持锁时间）
    std::vector<EventListener> toNotify;
    {
        std::lock_guard<std::mutex> lock(listenerMutex_);
        auto range = listeners_.equal_range(event.eventName);
        for (auto it = range.first; it != range.second; ++it) {
            toNotify.push_back(it->second);
        }
    }

    // 逐个通知
    for (const auto& listener : toNotify) {
        if (listener) {
            listener(event);
        }
    }
}

} // namespace rnoh_new
