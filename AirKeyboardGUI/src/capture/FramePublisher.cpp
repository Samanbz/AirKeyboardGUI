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
    if (FAILED(hr)) return hr;

    hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                             MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    if (FAILED(hr)) {
        attributes->Release();
        return hr;
    }

    hr = MFEnumDeviceSources(attributes, &devices, &deviceCount);
    if (FAILED(hr) || deviceCount == 0) {
        attributes->Release();
        return hr;
    }

    hr = devices[0]->ActivateObject(IID_PPV_ARGS(ppMediaSource));

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
    if (FAILED(hr)) return hr;

    hr = attributes->SetUINT32(MF_LOW_LATENCY, TRUE);
    if (FAILED(hr)) {
        attributes->Release();
        return hr;
    }

    hr = MFCreateSourceReaderFromMediaSource(mediaSource, attributes, ppSourceReader);

    attributes->Release();
    return hr;
}

HRESULT FramePublisher::configureOutputFormat() {
    IMFMediaType* mediaType = nullptr;

    HRESULT hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr)) return hr;

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr)) {
        mediaType->Release();
        return hr;
    }

    // My webcam uses NV12 format, TODO make this (automatically?) configurable
    hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    if (FAILED(hr)) {
        mediaType->Release();
        return hr;
    }

    hr = MFSetAttributeSize(mediaType, MF_MT_FRAME_SIZE, DEFAULT_FRAME_WIDTH, DEFAULT_FRAME_HEIGHT);
    if (FAILED(hr)) {
        mediaType->Release();
        return hr;
    }

    hr = sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mediaType);

    mediaType->Release();
    return hr;
}

FramePublisher::FramePublisher() : sourceReader(nullptr), frameWidth(), frameHeight(0) {
    if (instance != nullptr) {
        throw std::runtime_error("Only one FramePublisher instance allowed");
    }

    instance = this;

    HRESULT hr = initializeMediaFoundation();
    if (FAILED(hr)) {
        // TODO handle initialization error
    }
}

void FramePublisher::captureFrame() {
    if (!sourceReader) return;

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
        return;
    }

    if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
        return;  // TODO handle?
    }

    if (streamFlags & MF_SOURCE_READERF_STREAMTICK) {
        return;  // Ignore stream ticks
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