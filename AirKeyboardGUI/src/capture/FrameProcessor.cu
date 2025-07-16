#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#include "FrameProcessor.h"

// Optimized dimensions - divisible by 32 for warp alignment
constexpr int CROP_WIDTH = FrameProcessor::CROP_WIDTH;
constexpr int CROP_HEIGHT = FrameProcessor::CROP_HEIGHT;

__global__ void nv12ToRgbCropKernel(const uint8_t* __restrict__ nv12Data,
                                    uint8_t* __restrict__ rgbData,
                                    int srcWidth, int srcHeight,
                                    int cropX, int cropY) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= CROP_WIDTH || y >= CROP_HEIGHT) return;

    // Source coordinates (with horizontal and vertical flip)
    int srcX = srcWidth - 1 - (cropX + x);   // Flip horizontally
    int srcY = srcHeight - 1 - (cropY + y);  // Flip vertically

    // Y plane
    int yIndex = srcY * srcWidth + srcX;
    int yValue = nv12Data[yIndex];

    // UV plane (interleaved, half resolution)
    int uvIndex = srcHeight * srcWidth + (srcY / 2) * srcWidth + (srcX & ~1);
    int uValue = nv12Data[uvIndex];
    int vValue = nv12Data[uvIndex + 1];

    // YUV to RGB conversion (using integer math for speed)
    int c = yValue - 16;
    int d = uValue - 128;
    int e = vValue - 128;

// Clamp macro
#define CLAMP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)))

    int r = CLAMP((298 * c + 409 * e + 128) >> 8);
    int g = CLAMP((298 * c - 100 * d - 208 * e + 128) >> 8);
    int b = CLAMP((298 * c + 516 * d + 128) >> 8);

#undef CLAMP

    // Output RGB (interleaved)
    int rgbIndex = (y * CROP_WIDTH + x) * 3;
    rgbData[rgbIndex] = b;      // Blue first
    rgbData[rgbIndex + 1] = g;  // Green second
    rgbData[rgbIndex + 2] = r;  // Red last
}

// Host function callable from C++
extern "C" void launchNv12ToRgbCrop(const uint8_t* d_nv12,
                                    uint8_t* d_rgb,
                                    int srcWidth, int srcHeight,
                                    int cropX, int cropY,
                                    cudaStream_t stream) {
    dim3 blockSize(32, 16);  // 512 threads per block
    dim3 gridSize(
        (CROP_WIDTH + blockSize.x - 1) / blockSize.x,
        (CROP_HEIGHT + blockSize.y - 1) / blockSize.y);

    nv12ToRgbCropKernel<<<gridSize, blockSize, 0, stream>>>(
        d_nv12, d_rgb, srcWidth, srcHeight, cropX, cropY);
}