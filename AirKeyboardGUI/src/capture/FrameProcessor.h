#pragma once

#include <cuda_runtime.h>
#include <mfapi.h>
#include <windows.h>

#include <memory>

#include "../base/Publisher.h"
#include "../base/StreamSubscriber.h"
#include "../types.h"

// Forward declare CUDA function
extern "C" void launchNv12ToRgbCrop(
    const uint8_t* d_nv12,
    uint8_t* d_rgb,
    int srcWidth, int srcHeight,
    int cropX, int cropY,
    cudaStream_t stream);

/**
 * @brief GPU-accelerated frame processor that converts NV12 to RGB and crops.
 *
 * Subscribes to IMFSample frames from FramePublisher, processes them using CUDA,
 * and publishes ProcessedFrame objects containing RGB data with metadata.
 */
class FrameProcessor : public StreamSubscriber<IMFSample>, public Publisher<ProcessedFrame> {
public:
    static constexpr int CROP_WIDTH = 912;
    static constexpr int CROP_HEIGHT = 600;

private:
    // Source dimensions
    const int srcWidth = 1920;
    const int srcHeight = 1080;

    // CUDA resources
    cudaStream_t stream = nullptr;
    uint8_t* d_nv12 = nullptr;
    uint8_t* d_rgb = nullptr;

    // CPU buffers (pinned memory for fast transfers)
    uint8_t* h_rgbCrop = nullptr;

    // Crop position (bottom center)
    int cropX;
    int cropY;

    // Performance counter frequency for timestamp conversion
    LARGE_INTEGER frequency;

    bool cudaInitialized = false;

    /**
     * @brief Initialize CUDA resources
     */
    bool initializeCuda();

    /**
     * @brief Clean up CUDA resources
     */
    void cleanupCuda();

    /**
     * @brief Process incoming frame from FramePublisher
     */
    void update(std::shared_ptr<IMFSample> sample) override;

    FrameProcessor();

    ~FrameProcessor();

public:
    FrameProcessor(const FrameProcessor&) = delete;  // Delete copy constructor to enforce singleton pattern

    FrameProcessor& operator=(const FrameProcessor&) = delete;  // Delete assignment operator to enforce singleton pattern

    /**
     * @brief Singleton instance accessor
     */
    static FrameProcessor& getInstance();
};