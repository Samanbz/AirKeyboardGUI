#include "FrameProcessor.h"

bool FrameProcessor::initializeCuda() {
    cudaError_t err = cudaSetDevice(0);
    if (err != cudaSuccess) {
        OutputDebugStringA("Failed to set CUDA device\n");
        return false;
    }

    // Create stream for async operations
    err = cudaStreamCreate(&stream);
    if (err != cudaSuccess) {
        OutputDebugStringA("Failed to create CUDA stream\n");
        return false;
    }

    // Allocate device memory
    size_t nv12Size = srcWidth * srcHeight * 3 / 2;
    size_t rgbCropSize = CROP_WIDTH * CROP_HEIGHT * 3;

    err = cudaMalloc(&d_nv12, nv12Size);
    if (err != cudaSuccess) {
        OutputDebugStringA("Failed to allocate device memory for NV12\n");
        cleanupCuda();
        return false;
    }

    err = cudaMalloc(&d_rgb, rgbCropSize);
    if (err != cudaSuccess) {
        OutputDebugStringA("Failed to allocate device memory for RGB\n");
        cleanupCuda();
        return false;
    }

    // Allocate pinned host memory for better transfer performance
    err = cudaHostAlloc(&h_rgbCrop, rgbCropSize, cudaHostAllocDefault);
    if (err != cudaSuccess) {
        OutputDebugStringA("Failed to allocate pinned host memory\n");
        cleanupCuda();
        return false;
    }

    // Calculate crop position (bottom center)
    cropX = (srcWidth - CROP_WIDTH) / 2;
    cropY = srcHeight - CROP_HEIGHT;  // bottom

    cudaInitialized = true;
    return true;
}

void FrameProcessor::cleanupCuda() {
    if (stream) {
        cudaStreamSynchronize(stream);
        cudaStreamDestroy(stream);
        stream = nullptr;
    }

    if (d_nv12) {
        cudaFree(d_nv12);
        d_nv12 = nullptr;
    }

    if (d_rgb) {
        cudaFree(d_rgb);
        d_rgb = nullptr;
    }

    if (h_rgbCrop) {
        cudaFreeHost(h_rgbCrop);
        h_rgbCrop = nullptr;
    }

    cudaInitialized = false;
}

void FrameProcessor::update(std::shared_ptr<IMFSample> sample) {
    if (!sample || !cudaInitialized) return;

    // Get NV12 data from sample
    IMFMediaBuffer* buffer = nullptr;
    HRESULT hr = sample->ConvertToContiguousBuffer(&buffer);
    if (FAILED(hr)) return;

    BYTE* nv12Data = nullptr;
    DWORD dataLength = 0;
    hr = buffer->Lock(&nv12Data, nullptr, &dataLength);
    if (FAILED(hr)) {
        buffer->Release();
        return;
    }

    UINT64 captureTime = 0;
    sample->GetUINT64(MFSampleExtension_Timestamp, &captureTime);

    // Copy NV12 data to device
    size_t nv12Size = srcWidth * srcHeight * 3 / 2;
    cudaError_t err = cudaMemcpyAsync(d_nv12, nv12Data, nv12Size,
                                      cudaMemcpyHostToDevice, stream);

    buffer->Unlock();
    buffer->Release();

    if (err != cudaSuccess) {
        OutputDebugStringA("Failed to copy NV12 data to device\n");
        return;
    }

    // Launch kernel for crop and RGB conversion
    launchNv12ToRgbCrop(d_nv12, d_rgb, srcWidth, srcHeight, cropX, cropY, stream);

    // Copy result back to host
    size_t rgbSize = CROP_WIDTH * CROP_HEIGHT * 3;
    err = cudaMemcpyAsync(h_rgbCrop, d_rgb, rgbSize, cudaMemcpyDeviceToHost, stream);

    // Wait for all operations to complete
    cudaStreamSynchronize(stream);

    if (err != cudaSuccess) {
        OutputDebugStringA("Failed to copy RGB data from device\n");
        return;
    }

    // Create ProcessedFrame
    auto processedFrame = std::make_shared<ProcessedFrame>();

    // Fill header
    processedFrame->header.timestamp = (captureTime * 1000) / frequency.QuadPart;
    processedFrame->header.width = CROP_WIDTH;
    processedFrame->header.height = CROP_HEIGHT;
    processedFrame->header.dataSize = static_cast<UINT32>(rgbSize);

    // Copy RGB data
    processedFrame->data = std::make_unique<BYTE[]>(rgbSize);
    memcpy(processedFrame->data.get(), h_rgbCrop, rgbSize);

    // Publish the processed frame
    publish(processedFrame);
}

FrameProcessor& FrameProcessor::getInstance() {
    static FrameProcessor instance;
    return instance;
}

FrameProcessor::FrameProcessor() {
    QueryPerformanceFrequency(&frequency);

    if (!initializeCuda()) {
        throw std::runtime_error("Failed to initialize CUDA for frame processing");
    }
}

FrameProcessor::~FrameProcessor() {
    cleanupCuda();
}