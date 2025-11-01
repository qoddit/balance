#include "FileOperations.h"
#include <fstream>
#include <ctime>
#include <openssl/rand.h>

namespace Balance {

Result<std::vector<uint8_t>> FileOperations::readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return Error("Failed to open file: " + path.string(), -1);
    }

    auto size = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    if (!file) {
        return Error("Failed to read file: " + path.string(), -1);
    }

    file.close();
    return data;
}

Result<void> FileOperations::writeFile(
    const std::filesystem::path& path,
    const std::vector<uint8_t>& data,
    bool overwrite) {

    if (!overwrite && std::filesystem::exists(path)) {
        return Error("File already exists: " + path.string(), -1);
    }

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return Error("Failed to open file for writing: " + path.string(), -1);
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());

    if (!file) {
        return Error("Failed to write file: " + path.string(), -1);
    }

    file.close();
    return Result<void>();
}

Result<uint64_t> FileOperations::getFileSize(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return Error("File does not exist: " + path.string(), -1);
    }

    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);

    if (ec) {
        return Error("Failed to get file size: " + ec.message(), ec.value());
    }

    return size;
}

bool FileOperations::fileExists(const std::filesystem::path& path) {
    return std::filesystem::exists(path);
}

Result<void> FileOperations::createDirectory(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);

    if (ec) {
        return Error("Failed to create directory: " + ec.message(), ec.value());
    }

    return Result<void>();
}

Result<void> FileOperations::secureDelete(const std::filesystem::path& path, int passes) {
    if (!std::filesystem::exists(path)) {
        return Error("File does not exist: " + path.string(), -1);
    }

    auto sizeResult = getFileSize(path);
    if (sizeResult.isError()) {
        return Error(sizeResult.error().message, sizeResult.error().code);
    }

    auto size = sizeResult.value();

    // Overwrite file multiple times
    for (int pass = 0; pass < passes; ++pass) {
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) {
            return Error("Failed to open file for secure deletion", -1);
        }

        // Overwrite with random data
        std::vector<uint8_t> randomData(std::min<size_t>(size, 1024 * 1024));  // 1MB chunks

        for (uint64_t written = 0; written < size; written += randomData.size()) {
            size_t toWrite = std::min<size_t>(randomData.size(), size - written);
            RAND_bytes(randomData.data(), toWrite);

            file.seekp(written);
            file.write(reinterpret_cast<const char*>(randomData.data()), toWrite);
        }

        file.flush();
        file.close();
    }

    // Finally delete the file
    std::error_code ec;
    std::filesystem::remove(path, ec);

    if (ec) {
        return Error("Failed to delete file: " + ec.message(), ec.value());
    }

    return Result<void>();
}

Result<void> FileOperations::copyFile(
    const std::filesystem::path& source,
    const std::filesystem::path& dest,
    bool overwrite) {

    if (!std::filesystem::exists(source)) {
        return Error("Source file does not exist: " + source.string(), -1);
    }

    if (!overwrite && std::filesystem::exists(dest)) {
        return Error("Destination file already exists: " + dest.string(), -1);
    }

    std::error_code ec;
    auto options = overwrite ? std::filesystem::copy_options::overwrite_existing
                             : std::filesystem::copy_options::none;

    std::filesystem::copy(source, dest, options, ec);

    if (ec) {
        return Error("Failed to copy file: " + ec.message(), ec.value());
    }

    return Result<void>();
}

Result<void> FileOperations::moveFile(
    const std::filesystem::path& source,
    const std::filesystem::path& dest) {

    if (!std::filesystem::exists(source)) {
        return Error("Source file does not exist: " + source.string(), -1);
    }

    std::error_code ec;
    std::filesystem::rename(source, dest, ec);

    if (ec) {
        return Error("Failed to move file: " + ec.message(), ec.value());
    }

    return Result<void>();
}

Result<std::vector<std::filesystem::path>> FileOperations::listFiles(
    const std::filesystem::path& path,
    const std::string& pattern,
    bool recursive) {

    if (!std::filesystem::exists(path)) {
        return Error("Directory does not exist: " + path.string(), -1);
    }

    std::vector<std::filesystem::path> files;

    try {
        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    // TODO: Implement pattern matching
                    files.push_back(entry.path());
                }
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    // TODO: Implement pattern matching
                    files.push_back(entry.path());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        return Error("Failed to list files: " + std::string(e.what()), -1);
    }

    return files;
}

Result<FileOperations::FileInfo> FileOperations::getFileInfo(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return Error("File does not exist: " + path.string(), -1);
    }

    FileInfo info;

    std::error_code ec;
    info.size = std::filesystem::file_size(path, ec);
    if (ec) {
        info.size = 0;
    }

    auto ftime = std::filesystem::last_write_time(path, ec);
    if (!ec) {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        info.modified = std::chrono::system_clock::to_time_t(sctp);
    } else {
        info.modified = 0;
    }

    info.created = info.modified;  // Created time not reliably available on all platforms
    info.isDirectory = std::filesystem::is_directory(path);
    info.isReadable = true;  // Simplified
    info.isWritable = true;  // Simplified

    return info;
}

Result<void> FileOperations::streamFile(
    const std::filesystem::path& path,
    size_t chunkSize,
    ChunkCallback callback) {

    auto sizeResult = getFileSize(path);
    if (sizeResult.isError()) {
        return Error(sizeResult.error().message, sizeResult.error().code);
    }

    auto totalSize = sizeResult.value();

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return Error("Failed to open file: " + path.string(), -1);
    }

    std::vector<uint8_t> chunk(chunkSize);
    size_t offset = 0;

    while (offset < totalSize) {
        size_t toRead = std::min(chunkSize, static_cast<size_t>(totalSize - offset));

        file.read(reinterpret_cast<char*>(chunk.data()), toRead);
        if (!file && !file.eof()) {
            return Error("Failed to read file chunk", -1);
        }

        chunk.resize(toRead);

        auto result = callback(chunk, offset, totalSize);
        if (result.isError()) {
            return result;
        }

        offset += toRead;
        chunk.resize(chunkSize);
    }

    file.close();
    return Result<void>();
}

Result<void> FileOperations::writeFileChunked(
    const std::filesystem::path& path,
    uint64_t totalSize,
    size_t chunkSize,
    ChunkProvider callback) {

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return Error("Failed to open file for writing: " + path.string(), -1);
    }

    size_t offset = 0;

    while (offset < totalSize) {
        size_t toWrite = std::min(chunkSize, static_cast<size_t>(totalSize - offset));

        auto chunkResult = callback(offset, toWrite);
        if (chunkResult.isError()) {
            return Error(chunkResult.error().message, chunkResult.error().code);
        }

        auto& chunk = chunkResult.value();

        file.write(reinterpret_cast<const char*>(chunk.data()), chunk.size());
        if (!file) {
            return Error("Failed to write file chunk", -1);
        }

        offset += chunk.size();
    }

    file.close();
    return Result<void>();
}

} // namespace Balance
