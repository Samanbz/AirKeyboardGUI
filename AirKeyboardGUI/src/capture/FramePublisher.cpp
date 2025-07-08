#include "FramePublisher.h"

// Define static member
FramePublisher* FramePublisher::instance = nullptr;

HRESULT FramePublisher::initializeMediaFoundation() {
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return hr;

    IMFMediaSource* mediaSource = nullptr;
    hr = createCameraSource(&mediaSource);
    if (FAILED(hr)) return hr;

    hr = createSourceReader(mediaSource, &sourceReader);
    mediaSource->Release();
    if (FAILED(hr)) return hr;

    hr = configureOutputFormat();
    if (FAILED(hr)) return hr;

    return S_OK;
}

HRESULT FramePublisher::createCameraSource(IMFMediaSource** ppMediaSource) {
    IMFAttributes* attributes = nullptr;
    IMFActivate** devices = nullptr;
    UINT32 deviceCount = 0;

    HRESULT hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to create MF attributes\n");
        return hr;
    }

    hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                             MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    if (FAILED(hr)) {
        OutputDebugString(L"Failed to set device source attribute\n");
        attributes->Release();
        return hr;
    }

    hr = MFEnumDeviceSources(attributes, &devices, &deviceCount);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to enumerate video devices\n");
        attributes->Release();
        return hr;
    }

    if (deviceCount == 0) {
        OutputDebugString(L"No video capture devices found!\n");
        attributes->Release();
        throw std::runtime_error("No camera connected to system");
    }

    hr = devices[0]->ActivateObject(IID_PPV_ARGS(ppMediaSource));
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to activate camera device\n");
    }

    for (UINT32 i = 0; i < deviceCount; i++) {
        devices[i]->Release();
    }
    CoTaskMemFree(devices);
    attributes->Release();

    return hr;
}

HRESULT FramePublisher::createSourceReader(IMFMediaSource* mediaSource, IMFSourceReader** ppSourceReader) {
    IMFAttributes* attributes = nullptr;

    HRESULT hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to create reader attributes\n");
        return hr;
    }

    hr = attributes->SetUINT32(MF_LOW_LATENCY, TRUE);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to set low latency mode\n");
        attributes->Release();
        return hr;
    }

    hr = MFCreateSourceReaderFromMediaSource(mediaSource, attributes, ppSourceReader);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to create source reader from media source\n");
    }

    attributes->Release();
    return hr;
}

HRESULT FramePublisher::configureOutputFormat() {
    IMFMediaType* mediaType = nullptr;

    HRESULT hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to create media type\n");
        return hr;
    }

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to set major type to video\n");
        mediaType->Release();
        return hr;
    }

    hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to set video format to NV12 - camera may not support this format\n");
        mediaType->Release();
        return hr;
    }

    hr = MFSetAttributeSize(mediaType, MF_MT_FRAME_SIZE, DEFAULT_FRAME_WIDTH, DEFAULT_FRAME_HEIGHT);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to set frame size to 1920x1080\n");
        mediaType->Release();
        return hr;
    }

    hr = sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mediaType);
    if (FAILED(hr)) {
        OutputDebugString(L"Failed to set media type on source reader - camera may not support 1920x1080 NV12\n");
    }

    mediaType->Release();
    return hr;
}

FramePublisher::FramePublisher() : sourceReader(nullptr), frameWidth(DEFAULT_FRAME_WIDTH), frameHeight(DEFAULT_FRAME_HEIGHT) {
    if (instance != nullptr) {
        throw std::runtime_error("Only one FramePublisher instance allowed");
    }

    instance = this;

    HRESULT hr = initializeMediaFoundation();
    if (FAILED(hr)) {
        instance = nullptr;  // Reset instance on failure
        throw std::runtime_error("Failed to initialize MediaFoundation camera capture");
    }
}

void FramePublisher::captureFrame() {
    if (!sourceReader) {
        OutputDebugString(L"captureFrame called with null sourceReader\n");
        return;
    }

    static UINT32 consecutiveErrors = 0;

    // Synchronous frame capture
    DWORD streamIndex = 0;
    DWORD streamFlags = 0;
    LONGLONG timestamp = 0;
    IMFSample* rawSample = nullptr;
    HRESULT hr = sourceReader->ReadSample(
        MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        &streamIndex,
        &streamFlags,
        &timestamp,
        &rawSample);

    if (FAILED(hr)) {
        consecutiveErrors++;
        if (consecutiveErrors % 30 == 1) {  // Log every 30th error (once per second at 30fps)
            WCHAR msg[256];
            swprintf_s(msg, L"ReadSample failed with HRESULT: 0x%08X (consecutive errors: %u)\n", hr, consecutiveErrors);
            OutputDebugString(msg);
        }
        return;
    }

    consecutiveErrors = 0;  // Reset on success

    if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
        OutputDebugString(L"Camera stream ended unexpectedly\n");
        return;
    }

    if (streamFlags & MF_SOURCE_READERF_STREAMTICK) {
        return;  // Stream ticks are normal, don't log
    }

    if (rawSample) {
        LARGE_INTEGER perfCounter;
        QueryPerformanceCounter(&perfCounter);

        rawSample->SetUINT64(MFSampleExtension_Timestamp, perfCounter.QuadPart);

        auto sample = std::shared_ptr<IMFSample>(rawSample, [](IMFSample* p) {
            if (p) p->Release();
        });

        publish(sample);
    }
}

FramePublisher* FramePublisher::getInstance() {
    if (!instance) {
        // throwing here because it's important to ensure the class is initialized on the correct thread.
        throw std::runtime_error("FramePublisher instance not created yet");
    }
    return instance;
}

FramePublisher::~FramePublisher() {
    shutdown();
    if (sourceReader) {
        sourceReader->Release();
        sourceReader = nullptr;
    }
    MFShutdown();
    instance = nullptr;
}