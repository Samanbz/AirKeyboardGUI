#include "AirKeyboardGUI.h"

#include <debugapi.h>

#include "src/ThreadManager.h"
#include "src/capture/KeyEventPublisher.h"

using namespace std;

HWND g_hMainWindow = nullptr;
ThreadManager* g_threadManager = new ThreadManager();

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
           LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE: {
            if (g_threadManager) {
                g_threadManager->stop();
            }
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
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
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
        0, L"AirKeyboardWindowClass", L"Air Keyboard GUI",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, x, y,
        wndWidth, wndHeight, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(g_hMainWindow, nCmdShow);
    UpdateWindow(g_hMainWindow);

    g_threadManager->start();

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
