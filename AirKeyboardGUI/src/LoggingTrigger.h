#pragma once

#include <atomic>
#include <chrono>
#include <optional>

#include "./EventBus.h"
#include "./base/StreamSubscriber.h"
#include "types.h"

/**
 * @brief Singleton class that monitors keyboard input to trigger logging sessions.
 *
 * LoggingTrigger watches for specific key sequences (default: 3 space presses within 1 second)
 * to start/stop logging sessions. Also provides automatic session timeout functionality
 * to prevent indefinite logging.
 */
class LoggingTrigger : public StreamSubscriber<KeyEvent> {
private:
    /// Singleton instance pointer
    static LoggingTrigger* instance;

    /// Virtual key code that triggers logging (default: VK_SPACE)
    USHORT triggerKey = VK_SPACE;

    /// Number of trigger key presses required to activate
    int triggerKeyCount = 3;

    /// Maximum time between key presses to count as sequence
    std::chrono::milliseconds timeout = std::chrono::milliseconds(1000);

    /// Maximum duration for automatic session termination
    std::chrono::milliseconds autoStopTimeout = std::chrono::milliseconds(30000);  // 30 seconds

    /// Current count of consecutive trigger key presses
    int keyPressCount = 0;

    /// Timestamp of last trigger key press
    std::chrono::steady_clock::time_point lastKeyTime;

    /// Atomic flag indicating if logging is currently active
    std::atomic<bool> loggingActive{false};

    /// Optional timestamp when current logging session started
    std::optional<std::chrono::steady_clock::time_point> loggingStartTime;

    /**
     * @brief Private constructor for singleton pattern.
     */
    LoggingTrigger();

    /**
     * @brief Processes keyboard events to detect trigger sequences.
     * @param ke Shared pointer to keyboard event data
     *
     * Monitors for trigger key sequences and publishes TOGGLE_LOGGING events.
     * Resets sequence counter on timeout or other key presses.
     */
    void update(std::shared_ptr<KeyEvent> ke) override;

public:
    /**
     * @brief Gets the singleton instance of LoggingTrigger.
     * @return Reference to the LoggingTrigger instance
     */
    static LoggingTrigger& getInstance();

    /**
     * @brief Checks if logging is currently active.
     * @return true if logging session is active, false otherwise
     */
    bool isLoggingActive() const;

    /**
     * @brief Checks for automatic session timeout and stops logging if exceeded.
     * @return true if logging was automatically stopped, false otherwise
     *
     * Should be called periodically in application main loop to enforce timeouts.
     */
    bool checkAutoStop();

    /**
     * @brief Sets the automatic session timeout duration.
     * @param timeout New timeout duration in milliseconds
     */
    void setAutoStopTimeout(std::chrono::milliseconds timeout);

    /**
     * @brief Destructor resets singleton instance pointer.
     */
    ~LoggingTrigger();

    /// Deleted copy constructor to enforce singleton pattern
    LoggingTrigger(const LoggingTrigger&) = delete;

    /// Deleted assignment operator to enforce singleton pattern
    LoggingTrigger& operator=(const LoggingTrigger&) = delete;
};