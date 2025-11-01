#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <variant>
#include <optional>
#include <memory>

namespace Balance {

// Error type for Result<T>
struct Error {
    std::string message;
    int code;

    explicit Error(std::string msg, int errorCode = -1)
        : message(std::move(msg)), code(errorCode) {}
};

// Result type for error handling without exceptions
template<typename T>
class Result {
private:
    std::variant<T, Error> data;

public:
    // Construct with success value
    Result(T value) : data(std::move(value)) {}

    // Construct with error
    Result(Error error) : data(std::move(error)) {}

    // Check if result is success
    bool isOk() const {
        return std::holds_alternative<T>(data);
    }

    // Check if result is error
    bool isError() const {
        return std::holds_alternative<Error>(data);
    }

    // Get value (throws if error)
    T& value() & {
        return std::get<T>(data);
    }

    const T& value() const & {
        return std::get<T>(data);
    }

    T&& value() && {
        return std::get<T>(std::move(data));
    }

    // Get error (throws if success)
    const Error& error() const {
        return std::get<Error>(data);
    }

    // Get value or default
    T valueOr(T defaultValue) const {
        if (isOk()) {
            return value();
        }
        return std::move(defaultValue);
    }
};

// Specialization for void return type
template<>
class Result<void> {
private:
    std::optional<Error> errorData;

public:
    // Construct with success
    Result() : errorData(std::nullopt) {}

    // Construct with error
    Result(Error error) : errorData(std::move(error)) {}

    // Check if result is success
    bool isOk() const {
        return !errorData.has_value();
    }

    // Check if result is error
    bool isError() const {
        return errorData.has_value();
    }

    // Get error (throws if success)
    const Error& error() const {
        return *errorData;
    }
};

// Encryption key types
enum class KeyType {
    Symmetric,
    RSAPublic,
    RSAPrivate,
    Hybrid
};

// Encryption key structure
struct EncryptionKey {
    KeyType type;
    std::vector<uint8_t> data;
    std::string identifier;

    EncryptionKey(KeyType t, std::vector<uint8_t> keyData, std::string id = "")
        : type(t), data(std::move(keyData)), identifier(std::move(id)) {}
};

// Algorithm types
enum class AlgorithmType {
    Balance,    // Proprietary hybrid format
    AES256,     // AES-256-CBC
    RSA4096,    // RSA with 4096-bit keys
    Serpent,    // Serpent-256
    Blowfish,   // Blowfish-448
    ChaCha20,   // ChaCha20-Poly1305 AEAD
    Twofish,    // Twofish-256
    CAST256     // CAST-256 (CAST6)
};

// Algorithm information
struct AlgorithmInfo {
    AlgorithmType type;
    std::string name;
    std::string description;
    size_t keySize;          // Key size in bits
    size_t blockSize;        // Block size in bytes
    bool isFIPSApproved;

    AlgorithmInfo(AlgorithmType t, std::string n, std::string desc,
                  size_t keySz, size_t blockSz, bool fips)
        : type(t), name(std::move(n)), description(std::move(desc)),
          keySize(keySz), blockSize(blockSz), isFIPSApproved(fips) {}
};

// File metadata for encrypted files
struct FileMetadata {
    uint64_t originalSize;
    uint64_t timestamp;
    AlgorithmType algorithm;
    std::vector<uint8_t> salt;
    std::vector<uint8_t> iv;  // Initialization vector
    std::string originalName;

    FileMetadata()
        : originalSize(0), timestamp(0), algorithm(AlgorithmType::Balance) {}
};

// Balance file header structure
struct BalanceHeader {
    char magic[8];           // "BALANCE1"
    uint32_t version;        // Format version
    uint32_t flags;          // Encryption flags
    uint64_t originalSize;   // Original file size
    uint64_t timestamp;      // Creation timestamp
    uint8_t salt[16];        // Salt for key derivation
    uint8_t reserved[16];    // Reserved for future use

    static constexpr size_t HEADER_SIZE = 64;
    static constexpr const char* MAGIC = "BALANCE1";
    static constexpr uint32_t CURRENT_VERSION = 1;
};

// Progress callback for long operations
using ProgressCallback = std::function<void(size_t current, size_t total, const std::string& status)>;

// Message structure for chat system
struct Message {
    uint64_t id;
    uint64_t timestamp;
    std::string text;
    std::string sender;
    bool isEncrypted;

    Message()
        : id(0), timestamp(0), isEncrypted(false) {}

    Message(uint64_t msgId, uint64_t ts, std::string txt, std::string sndr, bool encrypted = false)
        : id(msgId), timestamp(ts), text(std::move(txt)), sender(std::move(sndr)), isEncrypted(encrypted) {}
};

// Configuration structure
struct Configuration {
    std::string defaultAlgorithm;
    std::string keyStorePath;
    std::string databasePath;
    size_t maxMemoryUsage;      // Maximum memory in bytes
    size_t streamingThreshold;   // File size threshold for streaming
    bool enableFIPS;
    bool enableAuditLog;

    Configuration()
        : defaultAlgorithm("balance"),
          keyStorePath("./keys"),
          databasePath("./balance.db"),
          maxMemoryUsage(200 * 1024 * 1024),  // 200MB
          streamingThreshold(100 * 1024 * 1024),  // 100MB
          enableFIPS(false),
          enableAuditLog(true) {}
};

} // namespace Balance
