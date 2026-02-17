# AirKeyboard: High-Performance Multimodal Data Acquisition Engine

![C++](https://img.shields.io/badge/C++-17-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)
![CUDA](https://img.shields.io/badge/CUDA-11.0%2B-76B900?style=flat-square&logo=nvidia&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Win32-0078D6?style=flat-square&logo=windows&logoColor=white)
![Architecture](https://img.shields.io/badge/Architecture-Event--Driven-orange?style=flat-square)
![Pipeline](https://img.shields.io/badge/Pipeline-Multimodal-blueviolet?style=flat-square)

This repository hosts the core data acquisition infrastructure for the AirKeyboard project...
This repository hosts the core data acquisition infrastructure for the AirKeyboard project. The system is designed to generate large-scale, temporally aligned datasets for vision-based typing in VR/XR environments.

It addresses the specific engineering challenge of synchronizing high-bandwidth video streams (NV12 raw buffers) with hardware interrupt-level input events, maintaining isochronous data logging without blocking the main application loop.

## Engineering Context

Deep learning models for fine motor tasks require datasets where input latency is deterministic. Standard high-level implementations (e.g., pure Python/OpenCV) often introduce variable latency due to the Global Interpreter Lock (GIL) and garbage collection pauses, causing jitter between the frame timestamp and the recorded keystroke.

To mitigate this, the architecture was implemented as a native Win32 application (C++17) with custom CUDA kernels, ensuring that the data acquisition path remains distinct from the visualization path.

## System Architecture

The application is built around a **Publisher-Subscriber** pattern that decouples the hardware interfaces from the data serialization logic.

### Hardware-Accelerated Image Processing

Processing raw 1080p NV12 webcam feeds on the CPU consumes significant cycles, which can easily lead to frame drops during intensive data collection. To solve this, the system offloads color space conversion entirely to the GPU.

The core of this pipeline is a custom CUDA kernel located in `src/capture/FrameProcessor.cu`. By utilizing integer arithmetic to avoid floating-point overhead and designing the kernel with warp-aligned memory access patterns, it maximizes memory coalescing during the read/write of YUV planes. This "zero-copy" approach ensures the CPU is never burdened with pixel-level manipulation.

### Asynchronous Event Bus

A critical requirement for this tool was that disk I/O should never block the UI thread (Win32 Message Pump). I implemented a thread-safe event bus in `src/EventBus.h` to handle this.

Components subscribe to specific `AppEvent` types, allowing for a clean separation of concerns. When the capture thread publishes a new frame, the `FrameLogger` serializes it on a dedicated background thread, while the UI consumes the same event for rendering. This architecture ensures that writing heavy CSVs or images to disk never stalls the next capture interval.

### Native Win32 Integration

Rather than relying on heavy UI frameworks like Qt or Electron, the application is built directly on the Windows API (Win32), as seen in `AirKeyboardGUI.cpp`. This choice minimizes the memory footprint and provides direct access to the OS message loop.

Lifecycle management is handled by a custom `ThreadManager`, which orchestrates the graceful spin-up and teardown of the worker threads (Camera Capture, Key Publisher, Logger), ensuring resources are released cleanly upon termination.

## Visual Verification

**Latency Monitoring & Overlay**
*The system provides a real-time confidence view by overlaying MediaPipe inference results onto the raw capture feed. This verifies that the visual data (hand landmarks) remains synchronized with the physical input events.*

https://github.com/user-attachments/assets/8b9e7415-3c79-4b4b-ac67-2d526cb7767a

**Usage Demo**
*A demonstration of the data collection workflow, showing the correlation between physical keystrokes and the captured stream.*

https://github.com/user-attachments/assets/c668ce5d-ddf4-4f02-b161-5a52670c4fa0

**Architecture**
<img width="4988" height="2670" alt="image" src="https://github.com/user-attachments/assets/0fefce65-b5ab-4b17-848c-3c4a07dd848f" />


## Build & Dependencies

The project relies on modern CMake features (Presets) to manage build configurations across different environments.

**Requirements:**

* **Toolchain:** MSVC (Visual Studio 2022)
* **Compute:** CUDA Toolkit 11.0+ (NVCC)
* **Host:** Windows 10/11 (Win32 API)

**Compilation:**

```bash
# 1. Configure using CMake Presets (Auto-detects CUDA)
cmake --preset x64-release

# 2. Build the Native Engine
cmake --build --preset x64-release

```

## Project Structure

* `src/capture/` - Hardware interfacing and CUDA kernels (`.cu`).
* `src/logging/` - High-performance file I/O for frames and CSVs.
* `src/base/` - Core abstractions (Publisher, Subscriber, EventBus).
* `scripts/` - Python interop scripts for MediaPipe inference.
