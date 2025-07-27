#include "TextContainer.h"

bool TextContainer::classRegistered = false;

void TextContainer::registerWindowClass() {
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
        throw std::runtime_error("Failed to register TextContainer window class");
    }
    classRegistered = true;
}

void TextContainer::updateDPIScale() {
    HDC hdc = GetDC(nullptr);
    dpiScale = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0f;
    ReleaseDC(nullptr, hdc);
}

void TextContainer::computeCharSize() {
    HDC hdc = GetDC(handle);
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    GetTextExtentPoint32W(hdc, L"A", 1, &charSize);
    SelectObject(hdc, oldFont);
    ReleaseDC(handle, hdc);
}

void TextContainer::computeCharPositions() {
    int linePadding = 5;
    POINTI start = {0, 0};

    std::vector<size_t> toRemove;  // Track indices to remove

    for (size_t i = 0; i < children.size(); i++) {
        size_t wordEnd = displayText.find_first_of(L" ", i);
        if (wordEnd == std::wstring::npos) {
            wordEnd = displayText.size();
        }

        bool charOutOfBound = start.x > size.width - charSize.cx;
        bool wordOutOfBound = start.x + ((wordEnd - i) * charSize.cx) > size.width;

        if (charOutOfBound || wordOutOfBound) {
            start.x = 0;
            start.y += charSize.cy + linePadding;
            if (start.y > size.height - charSize.cy) {
                break;
            }
            if (displayText[i] == L' ') {
                toRemove.push_back(i);
                continue;
            }
        }

        children[i]->setPosition(start.x, start.y);
        start.x += charSize.cx;
    }

    // Remove spaces at line starts (in reverse to maintain indices)
    for (auto it = toRemove.rbegin(); it != toRemove.rend(); ++it) {
        children.erase(children.begin() + *it);
    }
}

void TextContainer::update(std::shared_ptr<KeyEvent> ke) {
    if (!ke->pressed) return;

    if (caretPosition == children.size() && ke->vkey != VK_BACK) {
        requestTextChunk();
        return;
    }

    if (ke->vkey == VK_BACK) {
        if (caretPosition > 0) {
            caretPosition--;
            children[caretPosition]->reset();
        }
    } else if (ke->vkey == VK_RETURN) {
        // handle enter
    } else if (ke->vkey == VK_ESCAPE) {
        // handle escape
    } else {
        BYTE keyboardState[256];
        GetKeyboardState(keyboardState);

        WCHAR buffer[16];
        int result = ToUnicode(ke->vkey, ke->scanCode, keyboardState, buffer, 16, 0);

        if (result > 0) {
            WCHAR ch = buffer[0];
            if (iswprint(ch)) {
                if (caretPosition < children.size()) {
                    children[caretPosition]->logKeyStroke(ch);
                    caretPosition++;
                }
            }
        }
    }
    InvalidateRect(handle, nullptr, TRUE);
}

int TextContainer::calculateWidth() {
    RECT clientRect;
    GetClientRect(g_hMainWindow, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    return clientWidth - 2 * hPad;
}

int TextContainer::calculateHeight() {
    RECT clientRect;
    GetClientRect(g_hMainWindow, &clientRect);
    int clientHeight = clientRect.bottom - clientRect.top;
    return clientHeight * 0.6 - vPad;
}

void TextContainer::updateChildren() {
    children.clear();
    children.reserve(displayText.size());

    for (const WCHAR ch : displayText) {
        children.push_back(std::make_unique<ReactiveChar>(ch, 0, 0));
    }
}

void TextContainer::subscribeToEvents() {
    EventBus::getInstance().subscribe(AppEvent::START_LOGGING, [this]() {
        PostMessage(handle, WM_UPDATE_CHILDREN, 0, 0);
    });

    EventBus::getInstance().subscribe(AppEvent::STOP_LOGGING, [this]() {
        PostMessage(handle, WM_UPDATE_CHILDREN, 0, 0);
    });
}

void TextContainer::toggleDisplayText() {
    if (displayText == defaultText) {
        displayText = textContent;
    } else {
        displayText = defaultText;
    }
    caretPosition = 0;
    updateChildren();
    computeCharPositions();
    InvalidateRect(handle, nullptr, TRUE);
}

void TextContainer::requestTextChunk() {
    textContent = TextProvider::getInstance().getNextChunk();
    caretPosition = 0;
    updateChildren();
    computeCharPositions();
    InvalidateRect(handle, nullptr, TRUE);
}

TextContainer::TextContainer() : UIView(hPad, vPad, calculateWidth(), calculateHeight()) {
    registerWindowClass();
    updateDPIScale();

    float scaledSize = fontSize * dpiScale;
    font = CreateFontW(
        -scaledSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Reddit Mono");

    handle = CreateWindowExW(
        WS_EX_COMPOSITED, className, nullptr,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        position.x, position.y, size.width, size.height,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);

    SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    if (!handle) {
        throw std::runtime_error("Failed to create TextContainer window");
    }

    ShowWindow(handle, SW_SHOW);
    UpdateWindow(handle);  // Force window update

    computeCharSize();
    subscribeToEvents();
    SendMessage(handle, WM_SETFONT, (WPARAM)font, TRUE);
    requestTextChunk();
}

void TextContainer::drawSelf(HDC hdc) {
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    for (const auto& child : children) {
        child->drawSelf(hdc);
    }

    if (caretPosition >= children.size()) {
        SelectObject(hdc, oldFont);
        return;
    }

    auto [x, y] = children[caretPosition]->getPosition();
    MoveToEx(hdc, x, y, nullptr);
    LineTo(hdc, x, y + charSize.cy);
    SelectObject(hdc, oldFont);
}

LRESULT TextContainer::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(handle, &ps);
            drawSelf(hdc);
            EndPaint(handle, &ps);
            return 0;
        }
        case WM_UPDATE_CHILDREN:
            toggleDisplayText();
            return 0;
        default:
            return DefWindowProc(handle, uMsg, wParam, lParam);
    }
}