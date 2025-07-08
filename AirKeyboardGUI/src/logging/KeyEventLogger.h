#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "../base/BatchSubscriber.h"
#include "../types.h"

/**
 * @brief Batch processor that logs keyboard events to CSV file.
 *
 * KeyEventLogger receives KeyEvent objects and writes them to a CSV file
 * in batches for improved I/O performance. Records timestamp, virtual key code,
 * scan code, and press/release state for each keyboard event.
 */
class KeyEventLogger : public BatchSubscriber<KeyEvent, 100> {
private:
    /// Path to the CSV log file where events will be written
    std::filesystem::path logFilePath;

    /// Performance counter frequency for timestamp conversion to milliseconds
    LARGE_INTEGER frequency;

    /**
     * @brief Processes accumulated batch of key events by writing to CSV file.
     *
     * Inherited from BatchSubscriber. Opens log file in append mode,
     * drains the flush queue, and writes each event as CSV row with format:
     * timestamp_ms,virtual_key,scan_code,pressed(1/0)
     */
    void processBatch() override;

public:
    /**
     * @brief Constructs KeyEventLogger with specified output file path.
     * @param filePath Path to CSV file where key events will be logged
     *
     * Initializes performance counter frequency for accurate timestamp conversion.
     */
    KeyEventLogger(const std::filesystem::path& filePath);
};