#pragma once

//clang-format off
#include <windows.h>
//clang-format on

#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <shlwapi.h>

#include <stdexcept>

#include "../base/Publisher.h"

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")

constexpr int DEFAULT_FRAME_WIDTH = 1920;
constexpr int DEFAULT_FRAME_HEIGHT = 1080;

/**
 * @brief Singleton class that captures video frames from camera and publishes them to subscribers.
 *
 * FramePublisher manages MediaFoundation camera capture, configures the video format,
 * and provides frame data to registered subscribers. Uses singleton pattern to ensure
 * only one camera capture instance exists per application.
 */
class FramePublisher : public Publisher<IMFSample> {
private:
    /// MediaFoundation source reader for camera access
    IMFSourceReader* sourceReader;

    /// Frame width (currently unused, for future expansion)
    UINT32 frameWidth;

    /// Frame height (currently unused, for future expansion)
    UINT32 frameHeight;

    /// Singleton instance pointer
    static FramePublisher* instance;

    /**
     * @brief Initializes MediaFoundation and sets up camera capture pipeline.
     * @return HRESULT indicating success or failure
     *
     * Creates camera source, source reader, and configures output format.
     */
    HRESULT initializeMediaFoundation();

    /**
     * @brief Creates MediaFoundation camera source from available video capture devices.
     * @param ppMediaSource Output pointer to created media source
     * @return HRESULT indicating success or failure
     *
     * Enumerates video capture devices and activates the first available camera.
     */
    HRESULT createCameraSource(IMFMediaSource** ppMediaSource);

    /**
     * @brief Creates source reader with low-latency configuration.
     * @param mediaSource Input media source from camera
     * @param ppSourceReader Output pointer to created source reader
     * @return HRESULT indicating success or failure
     *
     * Configures source reader for minimal latency video capture.
     */
    HRESULT createSourceReader(IMFMediaSource* mediaSource, IMFSourceReader** ppSourceReader);

    /**
     * @brief Configures video output format to NV12 with specified dimensions.
     * @return HRESULT indicating success or failure
     *
     * Sets up NV12 format at DEFAULT_FRAME_WIDTH x DEFAULT_FRAME_HEIGHT resolution.
     */
    HRESULT configureOutputFormat();

public:
    /**
     * @brief Constructs FramePublisher and initializes camera capture.
     * @throws std::runtime_error if instance already exists or initialization fails
     *
     * Enforces singleton pattern and sets up MediaFoundation capture pipeline.
     */
    FramePublisher();

    /**
     * @brief Captures a single frame from camera and publishes to subscribers.
     *
     * Performs synchronous frame capture, adds performance counter timestamp,
     * and publishes frame data to all registered subscribers. Handles various
     * stream states including end-of-stream and stream ticks.
     */
    void captureFrame();

    /**
     * @brief Gets the singleton instance of FramePublisher.
     * @return Pointer to the FramePublisher instance
     * @throws std::runtime_error if instance not yet created
     *
     * Ensures instance exists before returning pointer for thread safety.
     */
    static FramePublisher* getInstance();

    /**
     * @brief Destructor cleans up MediaFoundation resources and resets singleton.
     *
     * Shuts down publisher, releases source reader, and calls MFShutdown.
     */
    ~FramePublisher();

    /// Deleted copy constructor to enforce singleton pattern
    FramePublisher(const FramePublisher&) = delete;

    /// Deleted assignment operator to enforce singleton pattern
    FramePublisher& operator=(const FramePublisher&) = delete;
};