#pragma once

//clang-format off
#include <windows.h>
//clang-format on

#include <memory>
#include <stdexcept>
#include <vector>

#include "../base/Publisher.h"
#include "../types.h"
#include "cassert"

/**
 * @brief Singleton class that captures global keyboard events and publishes them to subscribers.
 *
 * KeyEventPublisher installs a low-level keyboard hook to capture all keyboard input
 * system-wide and publishes KeyEvent objects to registered subscribers. Uses singleton
 * pattern to ensure only one keyboard hook exists per application.
 */
class KeyEventPublisher : public Publisher<KeyEvent> {
private:
    /// Handle to the installed keyboard hook
    HHOOK hookHandle;

    /// Singleton instance pointer
    static std::unique_ptr<KeyEventPublisher> instance;

    /**
     * @brief Static callback function for Windows keyboard hook.
     * @param nCode Hook code indicating how to process the message
     * @param wParam Message identifier (key down/up events)
     * @param lParam Pointer to keyboard input data
     * @return Result of hook processing
     *
     * Windows calls this function for every keyboard event. Forwards to
     * instance method if valid, otherwise passes to next hook in chain.
     */
    static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Processes keyboard hook events and publishes KeyEvent objects.
     * @param nCode Hook code indicating message type
     * @param wParam Message identifier for key state
     * @param lParam Pointer to KBDLLHOOKSTRUCT with key data
     * @return Result passed to next hook in chain
     *
     * Extracts keyboard data, adds performance counter timestamp,
     * creates KeyEvent object, and publishes to subscribers.
     */
    LRESULT handleKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam);

    /**
     * @brief Private constructor for singleton pattern.
     * @throws std::runtime_error if keyboard hook installation fails
     *
     * Installs low-level keyboard hook for global key capture.
     */
    KeyEventPublisher();

public:
    /**
     * @brief Destructor cleans up keyboard hook and resets singleton.
     *
     * Shuts down publisher, removes keyboard hook, and nullifies instance.
     */
    ~KeyEventPublisher();

    /**
     * @brief Gets the singleton instance of KeyEventPublisher.
     * @return Reference to the KeyEventPublisher instance
     *
     * Creates instance on first call and returns reference for subsequent calls.
     */
    static KeyEventPublisher& getInstance();

    /// Deleted copy constructor to enforce singleton pattern
    KeyEventPublisher(const KeyEventPublisher&) = delete;

    /// Deleted assignment operator to enforce singleton pattern
    KeyEventPublisher& operator=(const KeyEventPublisher&) = delete;
};