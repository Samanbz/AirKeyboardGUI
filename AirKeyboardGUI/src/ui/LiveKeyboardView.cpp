#include "LiveKeyboardView.h"

bool LiveKeyboardView::classRegistered = false;

void LiveKeyboardView::registerWindowClass() {
    if (classRegistered) {
        return;
    }
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = UIView::WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
    if (!RegisterClassExW(&wc)) {
        throw std::runtime_error("Failed to register LiveKeyboardView window class");
    }
    classRegistered = true;
}

void LiveKeyboardView::update(std::shared_ptr<ProcessedFrame> frame) {
    if (!frame || !frame->data) return;

    for (int y = 0; y < viewHeight; y++) {
        for (int x = 0; x < viewWidth; x++) {
            // Map to source coordinates
            int srcX = (x * frame->header.width) / viewWidth;
            int srcY = (y * frame->header.height) / viewHeight;

            int srcIndex = (srcY * frame->header.width + srcX) * 3;
            int dstIndex = (y * viewWidth + x) * 3;

            // Copy RGB values
            frameBuffer[dstIndex] = frame->data[srcIndex];
            frameBuffer[dstIndex + 1] = frame->data[srcIndex + 1];
            frameBuffer[dstIndex + 2] = frame->data[srcIndex + 2];
        }
    }

    frameDirty = true;
    InvalidateRect(handle, nullptr, TRUE);
    UpdateWindow(handle);
}

int LiveKeyboardView::calculateX() {
    RECT rootWindowRect;
    GetClientRect(g_hMainWindow, &rootWindowRect);
    int rootWidth = rootWindowRect.right - rootWindowRect.left;
    return rootWidth - viewWidth;  // Position at right edge of root window.
}

int LiveKeyboardView::calculateY() {
    RECT rootWindowRect;
    GetClientRect(g_hMainWindow, &rootWindowRect);
    int rootHeight = rootWindowRect.bottom - rootWindowRect.top;
    return rootHeight - viewHeight;  // Position at bottom edge of root window.
}

LiveKeyboardView::LiveKeyboardView() : UIView(calculateX(), calculateY(), viewWidth, viewHeight) {
    registerWindowClass();

    frameBuffer = std::make_unique<BYTE[]>(viewWidth * viewHeight * 3);

    handle = CreateWindowExW(
        WS_EX_COMPOSITED, className, nullptr,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        position.x, position.y, size.width, size.height,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);
    SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    if (!handle) {
        throw std::runtime_error("Failed to create LiveKeyboardView window");
    }
}

void LiveKeyboardView::drawSelf(HDC hdc) {
    if (!frameDirty) {
        return;
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size.width;
    bmi.bmiHeader.biHeight = -size.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(hdc, 0, 0, size.width, size.height, 0, 0, 0, size.height,
                      frameBuffer.get(), &bmi, DIB_RGB_COLORS);
}

LRESULT LiveKeyboardView::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(handle, &ps);
            drawSelf(hdc);
            EndPaint(handle, &ps);
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        default:
            return DefWindowProcW(handle, uMsg, wParam, lParam);
    }
}