#pragma once

//clang-format off
#include <windows.h>
//clang-format on

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief Manages external Python process for post-processing video frames.
 *
 * FramePostProcessor spawns and manages a Python worker process that monitors
 * a directory for .raw frame files, processes them (e.g., hand detection),
 * and handles graceful shutdown with signal files.
 */
class FramePostProcessor {
private:
    /// Process information for spawned Python worker
    PROCESS_INFORMATION process_info = {};

    /// Startup configuration for Python process
    STARTUPINFOA startup_info = {};

    /// Directory path that Python worker monitors for frame files
    std::string watch_dir;

    /// Flag indicating whether Python process is currently active
    bool process_active = false;

public:
    /**
     * @brief Constructs FramePostProcessor with target directory.
     * @param dir Directory path for Python worker to monitor
     *
     * Initializes process structures and sets up monitoring directory.
     */
    FramePostProcessor(const std::string& dir);

    /**
     * @brief Spawns Python worker process to monitor frame directory.
     * @return true if process created successfully, false otherwise
     *
     * Executes Python script with specified directory and worker count.
     * Logs process ID and status information for debugging.
     */
    bool SpawnWorker();

    /**
     * @brief Gracefully terminates Python worker process.
     *
     * Creates shutdown signal file, waits for graceful exit (30 seconds),
     * then forces termination if necessary. Cleans up process handles.
     */
    void terminateWorker();

    /**
     * @brief Checks if Python worker process is still running.
     * @return true if process is active and running, false otherwise
     *
     * Queries process exit code to determine current status.
     */
    bool isRunning();

    /**
     * @brief Destructor ensures Python worker is properly terminated.
     *
     * Calls terminateWorker() to clean up any running processes.
     */
    ~FramePostProcessor();
};