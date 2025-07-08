#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "../../config.h"

/**
 * @brief Singleton class that provides text chunks for typing practice from a file.
 *
 * TextProvider manages reading text content in manageable chunks while maintaining
 * progress across application sessions. It handles file I/O, position tracking,
 * and provides both sequential and preview access to text content.
 */
class TextProvider {
private:
    /// Path to the source text file
    std::filesystem::path textFilePath;

    /// Path to the progress tracking file
    std::filesystem::path progressFilePath;

    /// Current reading position within the text file
    std::streampos currentFilePosition = 0;

    /// Total size of the text file in bytes
    std::streampos fileSize = 0;

    /// Maximum number of words to include in a single chunk
    static constexpr size_t MAX_WORDS_PER_CHUNK = 100;

    /// Buffer size for file reading operations
    static constexpr size_t READ_BUFFER_SIZE = 8192;  // Read in 8KB chunks

    /**
     * @brief Determines and stores the total size of the text file.
     * @throws std::runtime_error if file cannot be opened
     */
    void initializeFileSize();

    /**
     * @brief Loads previously saved reading progress from disk.
     * If progress file doesn't exist or position is invalid, starts from beginning.
     */
    void loadProgress();

    /**
     * @brief Saves current reading position to progress file.
     * Allows resuming from the same position in future sessions.
     */
    void saveProgress();

    /**
     * @brief Reads the next chunk of text from the file.
     * @return Wide string containing up to MAX_WORDS_PER_CHUNK words
     *
     * Reads word by word until chunk size limit is reached or end of file.
     * Updates currentFilePosition to track reading progress.
     */
    std::wstring readChunkFromFile();

    /**
     * @brief Private constructor for singleton pattern.
     * @param filePath Path to the text file to read from
     * @throws std::runtime_error if text file doesn't exist
     *
     * Initializes file paths, loads existing progress, and validates file access.
     */
    TextProvider(const std::filesystem::path& filePath);

    /**
     * @brief Destructor saves current progress before cleanup.
     */
    ~TextProvider();

public:
    /**
     * @brief Gets the singleton instance of TextProvider.
     * @return Reference to the single TextProvider instance
     *
     * Creates instance on first call using TEXT_FILE_PATH from config.
     */
    static TextProvider& getInstance();

    /**
     * @brief Retrieves the next text chunk and advances reading position.
     * @return Wide string with next chunk of text, empty if no more text
     *
     * Consumes text from the file and saves progress automatically.
     */
    std::wstring getNextChunk();

    /**
     * @brief Previews the next text chunk without advancing position.
     * @return Wide string with next chunk of text, empty if no more text
     *
     * Allows looking ahead at upcoming text without consuming it.
     */
    std::wstring peekNextChunk();

    /**
     * @brief Checks if there is more text available to read.
     * @return true if more text exists, false if at end of file
     */
    bool hasMoreText() const;

    /**
     * @brief Resets reading position to the beginning of the file.
     * Saves the reset position to progress file.
     */
    void reset();

    /**
     * @brief Calculates reading progress as a decimal percentage.
     * @return Progress value between 0.0 (start) and 1.0 (complete)
     */
    double getProgress() const;

    /**
     * @brief Gets current byte position within the file.
     * @return Current file position as streampos
     */
    std::streampos getCurrentPosition() const;

    /**
     * @brief Gets total file size in bytes.
     * @return Total file size as streampos
     */
    std::streampos getTotalSize() const;

    /**
     * @brief Jumps to a specific position in the file.
     * @param position Target position within the file
     *
     * Validates position is within file bounds before seeking.
     * Useful for implementing bookmarks or chapter navigation.
     */
    void seekToPosition(std::streampos position);

    /**
     * @brief Gets reading progress as an integer percentage.
     * @return Progress percentage from 0 to 100
     *
     * Convenient for displaying progress bars or status indicators.
     */
    int getProgressPercentage() const;
};