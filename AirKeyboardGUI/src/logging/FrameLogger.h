#pragma once

#include <mfapi.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

#include "../base/BatchSubscriber.h"
#include "../types.h"

/**
 * @brief Batch processor that logs video frames to disk in binary format.
 *
 * FrameLogger receives IMFSample objects containing video frame data and writes
 * them to individual .raw files with metadata headers. Processes frames in batches
 * for improved I/O performance and maintains session information.
 */
class FrameLogger : public BatchSubscriber<IMFSample, 100> {
private:
    std::filesystem::path logDirectory;  /// Directory path where frame files will be written

    size_t frameCount = 0;                            /// Counter for generating sequential frame filenames
    std::chrono::steady_clock::time_point startTime;  /// Session start time for duration tracking
    LARGE_INTEGER frequency;                          /// Performance counter frequency for timestamp conversion

    /**
     * @brief Header structure written before each frame's raw data.
     * Contains timing and size information for frame reconstruction.
     */
    struct FrameHeader {
        UINT64 timestamp;  ///< Frame timestamp in milliseconds
        UINT32 dataSize;   ///< Size of frame data in bytes
    };

    /**
     * @brief Writes a single frame sample to disk as binary file.
     * @param sample MediaFoundation sample containing frame data
     *
     * Extracts frame data, creates header with timing information,
     * and writes both header and raw frame data to sequentially numbered file.
     */
    void writeSampleToDisk(IMFSample* sample);

    /**
     * @brief Processes accumulated batch of frames by writing to disk.
     *
     * Inherited from BatchSubscriber. Drains the flush queue and writes
     * each frame to individual files with proper cleanup.
     */
    void processBatch() override;

public:
    /**
     * @brief Constructs FrameLogger with specified output directory.
     * @param logDir Directory path where frame files will be written
     *
     * Creates session metadata file with timestamp and format information.
     * Initializes performance counter frequency for timing calculations.
     */
    FrameLogger(const std::filesystem::path& logDir);

    /**
     * @brief Destructor ensures all pending frames are written to disk.
     *
     * Calls flush() to process any remaining frames in the batch queue.
     */
    ~FrameLogger();
};