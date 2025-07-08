#include "KeyEventPublisher.h"

// Define static member
std::unique_ptr<KeyEventPublisher> KeyEventPublisher::instance = nullptr;

LRESULT CALLBACK KeyEventPublisher::KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && instance) {
        return instance->handleKeyboardHook(nCode, wParam, lParam);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT KeyEventPublisher::handleKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        LARGE_INTEGER perfCounter;
        QueryPerformanceCounter(&perfCounter);

        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;

        bool pressed = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        std::shared_ptr<KeyEvent> ke = std::make_shared<KeyEvent>(KeyEvent{
            static_cast<USHORT>(kb->vkCode),
            static_cast<USHORT>(kb->scanCode),
            pressed,
            perfCounter.QuadPart});

        publish(ke);
    }

    // Always call next hook to maintain system functionality
    return CallNextHookEx(hookHandle, nCode, wParam, lParam);
}

KeyEventPublisher::KeyEventPublisher() : Publisher(), hookHandle(nullptr) {
    // Install low-level keyboard hook
    hookHandle = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        KeyboardHookProc,
        GetModuleHandle(nullptr),
        0  // Global hook (all threads)
    );

    if (!hookHandle) {
        instance = nullptr;
        throw std::runtime_error("Failed to install keyboard hook");
    }
}

KeyEventPublisher::~KeyEventPublisher() {
    shutdown();
    if (hookHandle) {
        UnhookWindowsHookEx(hookHandle);
        hookHandle = nullptr;
    }
    instance = nullptr;
}

KeyEventPublisher& KeyEventPublisher::getInstance() {
    if (!instance) {
        instance = std::unique_ptr<KeyEventPublisher>(new KeyEventPublisher());
    }
    return *instance;
}