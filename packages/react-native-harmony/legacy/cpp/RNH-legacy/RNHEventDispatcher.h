/**
 * RNOH-New: Event Dispatcher
 *
 * Dispatches events from native side (ArkUI) to the JS side (React).
 * Handles event bubbling, capture, and custom event delivery.
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace rnoh_new {

/**
 * Event priority levels
 */
enum class RNHEventPriority {
    Synchronous,  // Dispatch immediately on current thread
    Asynchronous, // Dispatch on JS thread
    Deferred,     // Dispatch when JS is idle
};

/**
 * Base event class
 */
struct RNHEvent {
    std::string eventName;
    std::string payload;  // JSON-serialized event data
    int surfaceId = -1;
    int targetTag = -1;
    RNHEventPriority priority = RNHEventPriority::Asynchronous;
    int64_t timestamp = 0;
};

/**
 * Event dispatcher that routes events from native to JS
 */
class RNHEventDispatcher {
public:
    RNHEventDispatcher();
    ~RNHEventDispatcher();

    // Dispatch an event to JS
    void dispatchEvent(const RNHEvent& event);

    // Dispatch a touch event
    void dispatchTouchEvent(int surfaceId, int targetTag,
                            int action, float x, float y,
                            int64_t timestamp);

    // Dispatch a custom event
    void dispatchCustomEvent(int surfaceId, int targetTag,
                             const std::string& eventName,
                             const std::string& payload);

    // Register an event listener
    using EventListener = std::function<void(const RNHEvent&)>;
    void addEventListener(const std::string& eventName, EventListener listener);

    // Remove an event listener
    void removeEventListener(const std::string& eventName,
                             EventListener listener);

    // Set the JS event callback (called when event should go to JS)
    using JSEventCallback = std::function<void(const std::string& eventName,
                                                const std::string& payload)>;
    void setJSEventCallback(JSEventCallback callback);

private:
    // Process events on the JS thread
    void processEventQueue();

    // Notify all local listeners registered for the event name
    void notifyLocalListeners(const RNHEvent& event);

    // Event queue for async dispatch
    std::vector<RNHEvent> eventQueue_;
    std::mutex queueMutex_;

    // Event listeners
    std::unordered_multimap<std::string, EventListener> listeners_;
    std::mutex listenerMutex_;

    // JS callback
    JSEventCallback jsCallback_;
};

} // namespace rnoh_new
