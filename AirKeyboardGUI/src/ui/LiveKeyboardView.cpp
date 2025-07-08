#include "LiveKeyboardView.h"

#include "../capture/FramePublisher.h"

bool LiveKeyboardView::classRegistered = false;

void convertAndProcessFrame(const BYTE* nv12Data, int width, int height, BYTE* rgbData, int outputWidth, int outputHeight) {
    const BYTE* yPlane = nv12Data;
    const BYTE* uvPlane = nv12Data + (width * height);

    for (int y = 0; y < outputHeight; y++) {
        for (int x = 0; x < outputWidth; x++) {
            // Map output coordinates to full input image
            int origX = (x * width) / outputWidth;
            int origY = (y * height) / outputHeight;

            // Flip the X coordinate
            int flippedOrigX = width - 1 - origX;

            int yIndex = origY * width + flippedOrigX;
            int uvIndex = (origY / 2) * width + (flippedOrigX & ~1);

            int Y = yPlane[yIndex] - 16;
            int U = uvPlane[uvIndex + 1] - 128;
            int V = uvPlane[uvIndex] - 128;

            // Compute RGB with scaled coefficients for faster computation
            int R = (298 * Y + 409 * V + 128) >> 8;
            int G = (298 * Y - 100 * U - 208 * V + 128) >> 8;
            int B = (298 * Y + 516 * U + 128) >> 8;

            R = (R < 0) ? 0 : (R > 255) ? 255
                                        : R;
            G = (G < 0) ? 0 : (G > 255) ? 255
                                        : G;
            B = (B < 0) ? 0 : (B > 255) ? 255
                                        : B;

            // Sequential write for cache performance
            int rgbIndex = (y * outputWidth + x) * 3;
            rgbData[rgbIndex] = R;
            rgbData[rgbIndex + 1] = G;
            rgbData[rgbIndex + 2] = B;
        }
    }
}

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

void LiveKeyboardView::update(std::shared_ptr<IMFSample> sample) {
    if (!sample) {
        return;
    }
    IMFMediaBuffer* buffer = nullptr;
    HRESULT hr = sample->ConvertToContiguousBuffer(&buffer);
    if (FAILED(hr)) {
        return;  // TODO: handle error appropriately.
    }
    BYTE* data = nullptr;
    DWORD maxLength = 0, currentLength = 0;
    hr = buffer->Lock(&data, &maxLength, &currentLength);
    if (FAILED(hr) || !data || currentLength == 0) {
        buffer->Release();
        return;  // TODO: handle error appropriately.
    }

    convertAndProcessFrame(data, DEFAULT_FRAME_WIDTH, DEFAULT_FRAME_HEIGHT,
                           frameBuffer.get(), viewWidth, viewHeight);

    buffer->Unlock();
    buffer->Release();

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
    bmi.bmiHeader.biHeight = size.height;
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
