#include "FrameLogger.h"

void FrameLogger::writeFrameToDisk(ProcessedFrame* frame) {
    if (!frame || !frame->data) return;

    size_t requiredSize = sizeof(FrameHeader) + frame->header.dataSize;

    std::stringstream fileName;
    fileName << logDirectory.string() << "/frame_" << std::setw(6) << std::setfill('0') << frameCount++ << ".raw";

    std::ofstream file(fileName.str(), std::ios::binary);
    if (!file.is_open()) {
        return;  // TODO? Handle error appropriately
    }

    file.write(reinterpret_cast<const char*>(&frame->header), sizeof(FrameHeader));

    // Write frame data
    file.write(reinterpret_cast<const char*>(frame->data.get()), frame->header.dataSize);

    file.close();
}

void FrameLogger::processBatch() {
    UINT32 batchFrameCount = 0;
    while (!flushQueue.empty()) {
        std::shared_ptr<ProcessedFrame> frame = flushQueue.front();

        writeFrameToDisk(frame.get());
        batchFrameCount++;
        flushQueue.pop();
    }
}

FrameLogger::FrameLogger(const std::filesystem::path& logDir)
    : logDirectory(logDir), startTime(std::chrono::steady_clock::now()) {
    QueryPerformanceFrequency(&frequency);
}

FrameLogger::~FrameLogger() {
    flush();
}