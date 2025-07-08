#include "LoggingTrigger.h"

// Define static member
LoggingTrigger* LoggingTrigger::instance = nullptr;

LoggingTrigger::LoggingTrigger() : lastKeyTime(std::chrono::steady_clock::now()) {}

void LoggingTrigger::update(std::shared_ptr<KeyEvent> ke) {
    if (!ke || !ke->pressed) return;

    auto now = std::chrono::steady_clock::now();

    if (ke->vkey == triggerKey) {
        // Reset counter if timeout exceeded
        if (now - lastKeyTime > timeout) {
            keyPressCount = 0;
        }

        keyPressCount++;
        lastKeyTime = now;

        if (keyPressCount >= triggerKeyCount) {
            EventBus::getInstance().publish(AppEvent::TOGGLE_LOGGING);
            loggingActive = !loggingActive;

            if (loggingActive) {
                loggingStartTime = now;
            } else {
                loggingStartTime.reset();
            }

            keyPressCount = 0;
        }
    } else {
        // Any other key resets the counter
        keyPressCount = 0;
    }
}

LoggingTrigger& LoggingTrigger::getInstance() {
    static LoggingTrigger instance;
    return instance;
}

bool LoggingTrigger::isLoggingActive() const {
    return loggingActive;
}

bool LoggingTrigger::checkAutoStop() {
    if (!loggingActive || !loggingStartTime.has_value()) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    if (now - loggingStartTime.value() >= autoStopTimeout) {
        OutputDebugString(L"Auto-stopping logging due to timeout.\n");
        EventBus::getInstance().publish(AppEvent::STOP_LOGGING);
        loggingActive = false;
        loggingStartTime.reset();
        return true;
    }

    return false;
}

void LoggingTrigger::setAutoStopTimeout(std::chrono::milliseconds timeout) {
    autoStopTimeout = timeout;
}

LoggingTrigger::~LoggingTrigger() {
    instance = nullptr;
}