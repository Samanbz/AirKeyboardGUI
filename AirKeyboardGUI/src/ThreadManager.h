#pragma once

#include <windows.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <thread>

#include "../config.h"
#include "EventBus.h"
#include "LoggingTrigger.h"
#include "capture/FrameProcessor.h"
#include "capture/FramePublisher.h"
#include "capture/KeyEventPublisher.h"
#include "logging/FrameLogger.h"
#include "logging/FramePostProcessor.h"
#include "logging/KeyEventLogger.h"
#include "ui/LiveKeyboardView.h"
#include "ui/TextContainer.h"

/**
 * @brief Manages all application threads and coordinates their lifecycle.
 *
 * ThreadManager handles the creation, coordination, and cleanup of all threads
 * in the application including capture threads, UI threads, and logging threads.
 * Provides event-driven logging session management with automatic cleanup.
 */
class ThreadManager {
private:
    /// Thread for continuous frame capture from camera
    std::thread framePublisherThread;

    /// Thread for continuous frame procesing
    std::thread frameProcessorThread;

    /// Thread for global keyboard event capture
    std::thread keyEventPublisherThread;

    /// Thread for text input UI and typing interface
    std::thread textUiThread;

    /// Thread for live video display component
    std::thread liveKeyboardViewThread;

    /// Thread for keyboard event logging (started/stopped with sessions)
    std::thread keyLoggerThread;

    /// Thread for frame logging (started/stopped with sessions)
    std::thread frameLoggerThread;

    /// Thread for monitoring logging trigger sequences
    std::thread loggingTriggerThread;

    /// Flag indicating if core application threads should continue running
    std::atomic<bool> running = false;

    /// Flag indicating if logging session is currently active
    std::atomic<bool> logging = false;

    /// Promise to signal when key event publisher is ready for subscriptions
    std::promise<void> keyEventPublisherReady;

    /// Future to wait for key event publisher initialization
    std::future<void> keyEventPublisherFuture;

    /// Promise to signal when frame publisher is ready for subscriptions
    std::promise<void> framePublisherReady;

    /// Future to wait for frame publisher initialization
    std::future<void> framePublisherFuture;

    /**
     * @brief Sets up event bus subscriptions for logging control.
     *
     * Subscribes to START_LOGGING, STOP_LOGGING, and TOGGLE_LOGGING events
     * to manage logging session lifecycle.
     */
    void subscribeToEvents();

    /**
     * @brief Starts frame and keyboard capture threads.
     *
     * Initializes continuous capture from camera and keyboard with
     * proper timing and message loop handling.
     */
    void startCapturing();

    /**
     * @brief Starts a new logging session with timestamped directory.
     *
     * Creates session directory, spawns logging threads for both keyboard
     * events and video frames, and initializes post-processing pipeline.
     */
    void startLogging();

    /**
     * @brief Stops current logging session and cleans up resources.
     *
     * Joins logging threads and ensures all data is properly flushed
     * before terminating the session.
     */
    void stopLogging();

public:
    /**
     * @brief Constructs ThreadManager and sets up event subscriptions.
     */
    ThreadManager();

    /**
     * @brief Starts all core application threads.
     *
     * Initializes capture threads, UI threads, and logging trigger monitoring.
     * Sets up proper thread synchronization and message loop handling.
     */
    void start();

    /**
     * @brief Stops all threads and performs cleanup.
     *
     * Signals all threads to stop, waits for proper shutdown, and joins
     * all thread handles. Should be called before application exit.
     */
    void stop();
};