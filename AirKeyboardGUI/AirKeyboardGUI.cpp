#include "AirKeyboardGUI.h"

#include <debugapi.h>
#include <dwmapi.h>

#include "resource.h"
#include "src/EventBus.h"
#include "src/ThreadManager.h"
#include "src/capture/KeyEventPublisher.h"
#pragma comment(lib, "dwmapi.lib")

using namespace std;

HWND g_hMainWindow = nullptr;
ThreadManager g_threadManager;

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
           LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE: {
            g_threadManager.stop();
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        case WM_CTLCOLORSTATIC:
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEXW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"AirKeyboardWindowClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_KEYBOARD_ICON));
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_KEYBOARD_ICON));
    wc.cbSize = sizeof(WNDCLASSEXW);

    RegisterClassExW(&wc);
    // Place window in the center of the screen
    RECT rect;
    GetClientRect(GetDesktopWindow(), &rect);
    int wndWidth = 1600;
    int wndHeight = 1200;
    int screenWidth = rect.right - rect.left;
    int screenHeight = rect.bottom - rect.top;
    int x = (screenWidth - wndWidth) / 2;
    int y = (screenHeight - wndHeight) / 2;

    g_hMainWindow = CreateWindowExW(
        0, L"AirKeyboardWindowClass", L"AirKeyboard",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, x, y,
        wndWidth, wndHeight, nullptr, nullptr, hInstance, nullptr);

    // Some Window customizations
    COLORREF lightGray = RGB(230, 230, 230);
    DwmSetWindowAttribute(g_hMainWindow, DWMWA_CAPTION_COLOR, &lightGray, sizeof(lightGray));

    EventBus::getInstance().subscribe(AppEvent::START_LOGGING, []() {
        COLORREF red = RGB(255, 0, 0);
        DwmSetWindowAttribute(g_hMainWindow, DWMWA_BORDER_COLOR, &red, sizeof(red));
        UpdateWindow(g_hMainWindow);
    });
    EventBus::getInstance().subscribe(AppEvent::STOP_LOGGING, []() {
        COLORREF color = DWMWA_COLOR_DEFAULT;
        DwmSetWindowAttribute(g_hMainWindow, DWMWA_BORDER_COLOR, &color, sizeof(color));
        UpdateWindow(g_hMainWindow);
    });

    ShowWindow(g_hMainWindow, nCmdShow);
    UpdateWindow(g_hMainWindow);

    g_threadManager.start();

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
