#pragma once

#include <windows.h>

typedef struct {
    USHORT vkey;         // Virtual key code
    USHORT scanCode;     // Scan code of the key
    bool pressed;        // True if pressed, false if released
    LONGLONG timestamp;  // Event
} KeyEvent;
