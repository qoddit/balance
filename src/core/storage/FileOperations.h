#pragma once

#include "balance/Types.h"
#include <filesystem>
#include <functional>

namespace Balance {

/**
 * File operations with encryption support
 * Handles reading, writing, streaming for large files
 */
class FileOperations {
public:
    /**
     * Read entire file into memory
     * @param path File path
     * @return File contents or error
     */
    static Result<std::vector<uint8_t>> readFile(const std::filesystem::path& path);

    /**
     * Write data to file
     * @param path File path
     * @param data Data to write
     * @param overwrite Allow overwriting existing file
     * @return Success or error
     */
    static Result<void> writeFile(
        const std::filesystem::path& path,
        const std::vector<uint8_t>& data,
        bool overwrite = false);

    /**
     * Get file size
     * @param path File path
     * @return File size in bytes or error
     */
    static Result<uint64_t> getFileSize(const std::filesystem::path& path);

    /**
     * Check if file exists
     * @param path File path
     * @return true if exists
     */
    static bool fileExists(const std::filesystem::path& path);

    /**
     * Create directory (including parents)
     * @param path Directory path
     * @return Success or error
     */
    static Result<void> createDirectory(const std::filesystem::path& path);

    /**
     * Delete file securely (overwrite before deletion)
     * @param path File path
     * @param passes Number of overwrite passes (3 or 7)
     * @return Success or error
     */
    static Result<void> secureDelete(const std::filesystem::path& path, int passes = 3);

    /**
     * Copy file
     * @param source Source file path
     * @param dest Destination file path
     * @param overwrite Allow overwriting
     * @return Success or error
     */
    static Result<void> copyFile(
        const std::filesystem::path& source,
        const std::filesystem::path& dest,
        bool overwrite = false);

    /**
     * Move/rename file
     * @param source Source file path
     * @param dest Destination file path
     * @return Success or error
     */
    static Result<void> moveFile(
        const std::filesystem::path& source,
        const std::filesystem::path& dest);

    /**
     * List files in directory
     * @param path Directory path
     * @param pattern Optional glob pattern
     * @param recursive Include subdirectories
     * @return List of file paths or error
     */
    static Result<std::vector<std::filesystem::path>> listFiles(
        const std::filesystem::path& path,
        const std::string& pattern = "*",
        bool recursive = false);

    /**
     * Get file metadata
     * @param path File path
     * @return File info or error
     */
    struct FileInfo {
        uint64_t size;
        std::time_t modified;
        std::time_t created;
        bool isDirectory;
        bool isReadable;
        bool isWritable;
    };

    static Result<FileInfo> getFileInfo(const std::filesystem::path& path);

    /**
     * Stream file in chunks (for large files)
     * @param path File path
     * @param chunkSize Size of each chunk
     * @param callback Function called for each chunk
     * @return Success or error
     */
    using ChunkCallback = std::function<Result<void>(const std::vector<uint8_t>& chunk, size_t offset, size_t total)>;

    static Result<void> streamFile(
        const std::filesystem::path& path,
        size_t chunkSize,
        ChunkCallback callback);

    /**
     * Write file in chunks (for large data)
     * @param path File path
     * @param totalSize Total size to write
     * @param chunkSize Size of each chunk
     * @param callback Function called to provide each chunk
     * @return Success or error
     */
    using ChunkProvider = std::function<Result<std::vector<uint8_t>>(size_t offset, size_t size)>;

    static Result<void> writeFileChunked(
        const std::filesystem::path& path,
        uint64_t totalSize,
        size_t chunkSize,
        ChunkProvider callback);

private:
    static constexpr size_t DEFAULT_CHUNK_SIZE = 4 * 1024 * 1024;  // 4MB
};

} // namespace Balance
