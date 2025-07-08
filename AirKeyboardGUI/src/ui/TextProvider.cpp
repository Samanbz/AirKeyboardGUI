#include "TextProvider.h"

void TextProvider::initializeFileSize() {
    std::ifstream file(textFilePath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open text file: " + textFilePath.string());
    }
    fileSize = file.tellg();
    file.close();
}

void TextProvider::loadProgress() {
    std::ifstream file(progressFilePath);
    if (file.is_open()) {
        std::streampos pos;
        file >> reinterpret_cast<std::streamoff&>(pos);
        currentFilePosition = pos;
        file.close();

        // Ensure position is within file bounds
        if (currentFilePosition >= fileSize) {
            currentFilePosition = 0;
        }
    }
}

void TextProvider::saveProgress() {
    std::ofstream file(progressFilePath);
    if (file.is_open()) {
        file << currentFilePosition;
        file.close();
    }
}

std::wstring TextProvider::readChunkFromFile() {
    std::wifstream file(textFilePath);
    if (!file.is_open()) {
        return L"";
    }

    // Seek to current position
    file.seekg(currentFilePosition);
    if (file.eof()) {
        file.close();
        return L"";
    }

    std::wstring buffer;
    std::wstring word;
    size_t wordCount = 0;
    std::streampos chunkStartPos = currentFilePosition;

    // Read words until we have enough for a chunk
    while (wordCount < MAX_WORDS_PER_CHUNK && file >> word) {
        if (!buffer.empty()) {
            buffer += L" ";
        }
        buffer += word;
        wordCount++;
    }

    // Update current position
    currentFilePosition = file.tellg();
    if (file.eof()) {
        currentFilePosition = fileSize;
    }

    file.close();
    return buffer;
}

TextProvider::TextProvider(const std::filesystem::path& filePath)
    : textFilePath(filePath) {
    if (!std::filesystem::exists(textFilePath)) {
        throw std::runtime_error("Text file does not exist: " + textFilePath.string());
    }

    // Create progress file path (same directory as text file, with .progress extension)
    progressFilePath = textFilePath;
    progressFilePath.replace_extension(".progress");

    initializeFileSize();
    loadProgress();
}

TextProvider::~TextProvider() {
    saveProgress();
}

TextProvider& TextProvider::getInstance() {
    static TextProvider instance(TEXT_FILE_PATH);
    return instance;
}

std::wstring TextProvider::getNextChunk() {
    if (!hasMoreText()) {
        return L"";
    }

    std::wstring chunk = readChunkFromFile();
    saveProgress();
    return chunk;
}

std::wstring TextProvider::peekNextChunk() {
    if (!hasMoreText()) {
        return L"";
    }

    // Save current position
    std::streampos savedPosition = currentFilePosition;

    // Read chunk
    std::wstring chunk = readChunkFromFile();

    // Restore position
    currentFilePosition = savedPosition;

    return chunk;
}

bool TextProvider::hasMoreText() const {
    return currentFilePosition < fileSize;
}

void TextProvider::reset() {
    currentFilePosition = 0;
    saveProgress();
}

double TextProvider::getProgress() const {
    if (fileSize == 0) return 0.0;
    return static_cast<double>(currentFilePosition) / static_cast<double>(fileSize);
}

std::streampos TextProvider::getCurrentPosition() const {
    return currentFilePosition;
}

std::streampos TextProvider::getTotalSize() const {
    return fileSize;
}

void TextProvider::seekToPosition(std::streampos position) {
    if (position >= 0 && position < fileSize) {
        currentFilePosition = position;
        saveProgress();
    }
}

int TextProvider::getProgressPercentage() const {
    return static_cast<int>(getProgress() * 100);
}