#pragma once

#include <windows.h>

#include <memory>

typedef struct {
    USHORT vkey;         // Virtual key code
    USHORT scanCode;     // Scan code of the key
    bool pressed;        // True if pressed, false if released
    LONGLONG timestamp;  // Event
} KeyEvent;

#pragma pack(push, 1)
typedef struct {
    UINT64 timestamp;  // Frame timestamp in milliseconds
    UINT32 width;      // Frame width
    UINT32 height;     // Frame height
    UINT32 dataSize;   // Size of frame data in bytes
} FrameHeader;
#pragma pack(pop)

typedef struct {
    FrameHeader header;
    std::unique_ptr<BYTE[]> data;  // RGB data
} ProcessedFrame;