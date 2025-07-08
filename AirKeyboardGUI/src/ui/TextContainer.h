#pragma once

//clang-format off
#include <windows.h>
//clang-format on

#include <memory>
#include <string>
#include <vector>
#include "../EventBus.h"
#include "../base/StreamSubscriber.h"
#include "../base/UIView.h"
#include "../types.h"
#include "ReactiveChar.h"
#include "TextProvider.h"

/// Custom message to trigger children update asynchronously
#define WM_UPDATE_CHILDREN (WM_USER + 1)

/**
 * @brief Manages a collection of reactive characters for text display and input handling.
 *
 * TextContainer is responsible for:
 * - Containing and managing ReactiveChar objects
 * - Routing keystrokes to individual characters
 * - Handling text layout with word wrapping
 * - Drawing all characters within the container
 * - Managing typing progress and caret position
 */
class TextContainer : public UIView, public StreamSubscriber<KeyEvent> {
private:
    /// Default message displayed when not actively typing
    const std::wstring defaultText = L"Hit space 3 times to start logging...";

    /// Current text content from TextProvider
    std::wstring textContent;

    /// Currently displayed text (either defaultText or textContent)
    std::wstring displayText = defaultText;

    /// Current position of the typing caret
    size_t caretPosition = 0;

    /// Font handle for text rendering
    HFONT font;

    /// Base font size in points
    float fontSize = 18.0f;

    /// DPI scaling factor for high-DPI displays
    float dpiScale = 1.0f;

    /// Dimensions of a single character in the current font
    SIZE charSize;

    /// Horizontal padding from container edges
    static constexpr int hPad = 120;

    /// Vertical padding from container edges
    static constexpr int vPad = 100;

    /// Collection of ReactiveChar objects representing each character
    std::vector<ReactiveChar*> children;

    /// Windows class name for this container type
    static constexpr LPCWSTR className = L"TextContainerClass";

    /// Tracks whether the Windows class has been registered
    static bool classRegistered;

    /**
     * @brief Registers the Windows class for TextContainer instances.
     * Only registers once per application lifetime.
     */
    static void registerWindowClass();

    /**
     * @brief Updates DPI scaling factor based on current display settings.
     */
    void updateDPIScale();

    /**
     * @brief Calculates character dimensions for the current font and DPI.
     */
    void computeCharSize();

    /**
     * @brief Positions all characters with word wrapping and line breaks.
     * Handles overflow by moving to next line and removes spaces at line starts.
     */
    void computeCharPositions();

    /**
     * @brief Processes keyboard input events.
     * @param ke Shared pointer to the key event data
     *
     * Handles:
     * - Character input and typing progress
     * - Backspace for corrections
     * - Special keys (Enter, Escape)
     * - Automatic text chunk requests when reaching end
     */
    void update(std::shared_ptr<KeyEvent> ke) override;

    /**
     * @brief Calculates container width based on main window size.
     * @return Width in pixels, accounting for horizontal padding
     */
    int calculateWidth();

    /**
     * @brief Calculates container height based on main window size.
     * @return Height in pixels (60% of main window minus vertical padding)
     */
    int calculateHeight();

    /**
     * @brief Creates new ReactiveChar objects for current display text.
     * Cleans up existing characters and creates fresh instances.
     */
    void updateChildren();

    /**
     * @brief Sets up event bus subscriptions for logging state changes.
     * Listens for TOGGLE_LOGGING and STOP_LOGGING events.
     */
    void subscribeToEvents();

    /**
     * @brief Switches between default text and actual content.
     * Resets caret position and updates character layout.
     */
    void toggleDisplayText();

    /**
     * @brief Requests next text chunk from TextProvider.
     * Updates content and resets typing state for new text.
     */
    void requestTextChunk();

public:
    /**
     * @brief Constructs TextContainer with calculated dimensions.
     *
     * Initializes:
     * - Windows class registration
     * - DPI-aware font creation
     * - Window creation and event subscriptions
     * - Initial text chunk loading
     */
    TextContainer();

    /**
     * @brief Renders all characters and the typing caret.
     * @param hdc Device context for drawing operations
     *
     * Draws each ReactiveChar and shows caret at current position.
     */
    void drawSelf(HDC hdc) override;

    /**
     * @brief Handles Windows messages for the container.
     * @param uMsg Message identifier
     * @param wParam Message parameter
     * @param lParam Message parameter
     * @return Message handling result
     *
     * Processes:
     * - WM_PAINT for rendering
     * - WM_UPDATE_CHILDREN for asynchronous text updates
     */
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};