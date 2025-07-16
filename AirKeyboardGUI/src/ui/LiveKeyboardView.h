#pragma once

//clang-format off
#include <windows.h>
//clang-format on

#include <debugapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <atomic>
#include <memory>
#include <queue>

#include "../base/StreamSubscriber.h"
#include "../base/UIView.h"
#include "../types.h"

/**
 * @brief Real-time video display component for keyboard/hand tracking visualization.
 *
 * LiveKeyboardView receives video frames from a MediaFoundation source, converts them
 * from NV12 to RGB format, applies mirroring, and displays the result in real-time.
 * Positioned at the bottom-right corner of the main window for overlay-style display.
 */
class LiveKeyboardView : public UIView, public StreamSubscriber<ProcessedFrame> {
private:
    /// Fixed display width in pixels
    static constexpr int viewWidth = 720;

    /// Fixed display height in pixels
    static constexpr int viewHeight = 405;

    /// Windows class name for this view type
    static constexpr LPCWSTR className = L"liveKeyboardViewClass";

    /// Tracks whether the Windows class has been registered
    static bool classRegistered;

    /// RGB frame buffer for converted video data
    std::unique_ptr<BYTE[]> frameBuffer;

    /// Flag indicating frame buffer contains new data requiring redraw
    std::atomic<bool> frameDirty = false;

    /**
     * @brief Registers the Windows class for LiveKeyboardView instances.
     * Only registers once per application lifetime to avoid duplicate registration.
     */
    static void registerWindowClass();

    /**
     * @brief Processes incoming processed video frames.
     * @param sample Shared pointer to ProcessedFrame containing frame data
     *
     * Scales and stores processed frame into frame buffer, and triggers window redraw.
     */
    void update(std::shared_ptr<ProcessedFrame> sample) override;

    /**
     * @brief Calculates X position for right-edge alignment.
     * @return X coordinate placing view at right edge of main window
     *
     * Positions the view flush against the right border of the parent window
     * for overlay-style display that doesn't interfere with main content.
     */
    int calculateX();

    /**
     * @brief Calculates Y position for bottom-edge alignment.
     * @return Y coordinate placing view at bottom edge of main window
     *
     * Positions the view flush against the bottom border of the parent window
     * creating a picture-in-picture style display in the corner.
     */
    int calculateY();

public:
    /**
     * @brief Constructs LiveKeyboardView with automatic positioning and setup.
     * @throws std::runtime_error if window creation fails
     *
     * Initializes:
     * - Windows class registration
     * - RGB frame buffer allocation
     * - Child window creation with calculated position
     * - MediaFoundation integration setup
     */
    LiveKeyboardView();

    /**
     * @brief Renders the current video frame to the device context.
     * @param hdc Handle to the device context for drawing
     *
     * Only performs drawing when frameDirty flag indicates new frame data.
     * Uses device-independent bitmap functions for efficient RGB data display.
     * Handles proper bitmap header setup for 24-bit RGB format.
     */
    void drawSelf(HDC hdc) override;

    /**
     * @brief Handles Windows messages for the video view.
     * @param uMsg Message identifier
     * @param wParam Message parameter
     * @param lParam Message parameter
     * @return Message handling result
     *
     * Processes:
     * - WM_PAINT for frame rendering
     * - WM_DESTROY for cleanup and application termination
     * - Other messages delegated to default handler
     */
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};