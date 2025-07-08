#pragma once

#include <windows.h>
#include <stdexcept>
#include <string>
#include "../../globals.h"

typedef struct {
    int x;
    int y;
} POINTI;

typedef struct {
    int width;
    int height;
} SIZEI;

/*
 * @brief Computes a RECT structure from a position and size.
 *
 * I know 'getRect' would've been funnier.
 */
static RECT computeRect(const POINTI& position, const SIZEI& size) {
    RECT rect;
    rect.left = position.x;
    rect.top = position.y;
    rect.right = position.x + size.width;
    rect.bottom = position.y + size.height;
    return rect;
}

/**
 * @brief The abstraction of a view on the screen.
 *
 * Creates itself upon initialization with the root window as parent.

 * Draws itself in the given rectangle.
 */
class UIView {
protected:
    const HWND parent = g_hMainWindow;
    HWND handle = nullptr;
    POINTI position;
    SIZEI size;

public:
    UIView(int x = 0, int y = 0, int width = 0, int height = 0)
        : position({x, y}), size({width, height}) {}
    virtual ~UIView() = default;

    virtual void drawSelf(HDC hdc) = 0;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        UIView* instance = reinterpret_cast<UIView*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (instance) {
            return instance->handleMessage(uMsg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        return DefWindowProc(handle, uMsg, wParam, lParam);
    };

    HWND getHandle() const {
        return handle;
    }
};

// then figure out whether drawing is taking place correctly.
