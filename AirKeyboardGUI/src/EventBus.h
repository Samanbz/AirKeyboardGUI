#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <vector>

/**
 * @brief Application event types for inter-component communication.
 */
enum class AppEvent {
    START_LOGGING,   ///< Begin data logging session
    STOP_LOGGING,    ///< End data logging session
    TOGGLE_LOGGING,  ///< Switch logging state (start if stopped, stop if started)
};

/**
 * @brief Thread-safe singleton event bus for decoupled component communication.
 *
 * EventBus provides a publish-subscribe pattern allowing components to communicate
 * without direct dependencies. Components can subscribe to events and publish events
 * that trigger callbacks in all registered listeners.
 */
class EventBus {
private:
    /// Map of event types to their registered callback functions
    std::map<AppEvent, std::vector<std::function<void()>>> listeners;

    /// Mutex for thread-safe access to listeners map
    mutable std::mutex listenersMutex;

    /**
     * @brief Private constructor for singleton pattern.
     */
    EventBus() = default;

public:
    /**
     * @brief Gets the singleton instance of EventBus.
     * @return Reference to the EventBus instance
     */
    static EventBus& getInstance();

    /**
     * @brief Subscribes a callback function to an event type.
     * @param event The event type to listen for
     * @param callback Function to call when event is published
     *
     * Thread-safe operation that adds callback to the event's listener list.
     */
    void subscribe(AppEvent event, std::function<void()> callback);

    /**
     * @brief Publishes an event, triggering all registered callbacks.
     * @param event The event type to publish
     *
     * Thread-safe operation that calls all callbacks registered for the event type.
     */
    void publish(AppEvent event);

    /**
     * @brief Removes all callbacks for a specific event type.
     * @param event The event type to clear
     */
    void unsubscribe(AppEvent event);

    /**
     * @brief Removes all event listeners from the bus.
     */
    void clear();

    EventBus(const EventBus&) = delete;  // Deleted copy constructor to enforce singleton pattern

    EventBus& operator=(const EventBus&) = delete;  // Deleted assignment operator to enforce singleton pattern
};