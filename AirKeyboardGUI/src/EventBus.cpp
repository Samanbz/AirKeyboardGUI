#include "EventBus.h"

EventBus& EventBus::getInstance() {
    static EventBus instance;
    return instance;
}

void EventBus::subscribe(AppEvent event, std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(listenersMutex);
    listeners[event].push_back(callback);
}

void EventBus::publish(AppEvent event) {
    std::lock_guard<std::mutex> lock(listenersMutex);

    if (listeners.find(event) != listeners.end()) {
        for (auto& callback : listeners[event]) {
            callback();
        }
    }
}

void EventBus::unsubscribe(AppEvent event) {
    std::lock_guard<std::mutex> lock(listenersMutex);
    listeners.erase(event);
}

void EventBus::clear() {
    std::lock_guard<std::mutex> lock(listenersMutex);
    listeners.clear();
}