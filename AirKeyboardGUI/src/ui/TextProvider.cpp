#include "TextProvider.h"

#include <codecvt>  // Deprecated in C++17, but suitable here
#include <locale>

// Helper to create a UTF-8 locale
static std::locale utf8_locale() {
    return std::locale(std::locale(), new std::codecvt_utf8<wchar_t>());
}

void TextProvider::initializeFileSize() {
    // Use wifstream and the UTF-8 locale to correctly determine the file's end position
    std::wifstream file(textFilePath, std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open text file: " + textFilePath.string());
    }
    file.imbue(utf8_locale());
    fileSize = file.tellg();
    file.close();
}

void TextProvider::loadProgress() {
    std::ifstream file(progressFilePath);
    if (file.is_open()) {
        std::streamoff pos_off = 0;
        // Safely read the position as a stream offset value
        file >> pos_off;
        currentFilePosition = std::streampos(pos_off);
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
        // streampos can be converted to streamoff, which has a standard operator<<
        file << static_cast<std::streamoff>(currentFilePosition);
        file.close();
    }
}

std::wstring TextProvider::readChunkFromFile() {
    std::wifstream file(textFilePath);
    if (!file.is_open()) {
        return L"";
    }

    // Imbue the stream with the UTF-8 locale
    file.imbue(utf8_locale());

    // Seek to the last known valid position
    file.seekg(currentFilePosition);
    if (file.eof()) {
        file.close();
        return L"";
    }

    std::wstring buffer;
    std::wstring word;
    size_t wordCount = 0;

    while (wordCount < MAX_WORDS_PER_CHUNK && file >> word) {
        if (!buffer.empty()) {
            buffer += L' ';
        }
        buffer += word;
        wordCount++;
    }

    // Update the current position for the next chunk
    currentFilePosition = file.tellg();
    if (file.eof()) {
        // If we've reached the end, set position to the file size
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
    loadProgress();
    std::wstring chunk = readChunkFromFile();
    saveProgress();
    return chunk;
}

std::wstring TextProvider::peekNextChunk() {
    if (!hasMoreText()) {
        return L""
;
    }
    loadProgress();
    std::streampos savedPosition = currentFilePosition;
    std::wstring chunk = readChunkFromFile();
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
    // This progress might not be perfectly linear with the number of characters
    // due to the variable-length nature of UTF-8, but it's a good approximation.
    return static_cast<double>(static_cast<std::streamoff>(currentFilePosition)) /
           static_cast<double>(static_cast<std::streamoff>(fileSize));
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