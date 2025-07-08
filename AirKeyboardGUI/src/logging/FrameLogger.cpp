#include "FrameLogger.h"

void FrameLogger::writeSampleToDisk(IMFSample* sample) {
    // Get timestamp
    LONGLONG sampleTime = 0;
    sample->GetSampleTime(&sampleTime);

    // Get frame data
    IMFMediaBuffer* buffer = nullptr;
    if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer))) {
        BYTE* data = nullptr;
        DWORD dataLength = 0;

        if (SUCCEEDED(buffer->Lock(&data, nullptr, &dataLength))) {
            std::stringstream fileName;
            fileName << logDirectory.string() << "/frame_" << std::setw(6) << std::setfill('0') << frameCount++ << ".raw";

            std::ofstream file(fileName.str(), std::ios::binary);
            if (!file.is_open()) {
                buffer->Unlock();
                buffer->Release();
                return;  // Handle error appropriately
            }

            UINT64 captureTime = 0;
            sample->GetUINT64(MFSampleExtension_Timestamp, &captureTime);

            //// Write frame header
            FrameHeader frameHeader;
            frameHeader.timestamp = (captureTime * 1000) / frequency.QuadPart;
            frameHeader.dataSize = dataLength;

            file.write(reinterpret_cast<const char*>(&frameHeader), sizeof(FrameHeader));

            // Write frame data
            file.write(reinterpret_cast<const char*>(data), dataLength);

            file.close();
            buffer->Unlock();
        }
        buffer->Release();
    }
}

void FrameLogger::processBatch() {
    UINT32 batchFrameCount = 0;
    while (!flushQueue.empty()) {
        std::shared_ptr<IMFSample> sample = flushQueue.front();

        writeSampleToDisk(sample.get());
        batchFrameCount++;
        flushQueue.pop();
    }
}

FrameLogger::FrameLogger(const std::filesystem::path& logDir)
    : logDirectory(logDir), startTime(std::chrono::steady_clock::now()) {
    std::filesystem::path metaPath = logDirectory / "session_info.txt";
    std::ofstream metaFile(metaPath);
    if (metaFile.is_open()) {
        auto time = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(time);
        metaFile << "Session started: " << std::ctime(&time_t);
        metaFile << "Frame format: NV12\n";
        metaFile << "Frame size: 1920x1080\n";
        metaFile.close();
    }
    QueryPerformanceFrequency(&frequency);
}

FrameLogger::~FrameLogger() {
    flush();
}