#include "KeyEventLogger.h"

void KeyEventLogger::processBatch() {
    std::ofstream logFile(logFilePath, std::ios::app);
    if (!logFile.is_open()) {
        return;
    }

    while (!flushQueue.empty()) {
        auto keyEvent = flushQueue.front();
        flushQueue.pop();

        // Convert timestamp to milliseconds
        LONGLONG timestampMs = (keyEvent->timestamp * 1000) / frequency.QuadPart;

        // CSV format: timestamp,vkey,scancode,pressed
        logFile << timestampMs << ","
                << keyEvent->vkey << ","
                << keyEvent->scanCode << ","
                << (keyEvent->pressed ? "1" : "0") << "\n";
    }

    logFile.flush();
}

KeyEventLogger::KeyEventLogger(const std::filesystem::path& filePath) : logFilePath(filePath) {
    QueryPerformanceFrequency(&frequency);
}