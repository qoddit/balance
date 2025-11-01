#pragma once

#include "EncryptionEngine.h"
#include "AESEncryption.h"
#include "RSAEncryption.h"
#include "balance/Types.h"
#include <filesystem>

namespace Balance {

/**
 * Proprietary Balance encryption format
 * Hybrid encryption combining XOR + AES-256 + RSA-4096
 *
 * File structure:
 * [Header: 64 bytes]
 * [RSA-encrypted XOR key: 512 bytes]
 * [AES-encrypted data: variable]
 * [SHA-256 integrity hash: 32 bytes]
 */
class BalanceFormat : public EncryptionEngine {
public:
    BalanceFormat();
    ~BalanceFormat() override = default;

    // EncryptionEngine interface (for data encryption)
    Result<std::vector<uint8_t>> encrypt(
        const std::vector<uint8_t>& data,
        const EncryptionKey& key) override;

    Result<std::vector<uint8_t>> decrypt(
        const std::vector<uint8_t>& data,
        const EncryptionKey& key) override;

    AlgorithmInfo getInfo() const override;

    Result<EncryptionKey> generateKey() override;

    bool validateKey(const EncryptionKey& key) const override;

    /**
     * Encrypt a file to .balance format
     * @param inputPath Path to input file
     * @param outputPath Path to output .balance file (optional, defaults to input + ".balance")
     * @param password User password for key derivation
     * @param progressCallback Optional progress callback
     * @return Success or error
     */
    static Result<void> encryptFile(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::string& password,
        ProgressCallback progressCallback = nullptr);

    /**
     * Decrypt a .balance file
     * @param inputPath Path to .balance file
     * @param outputPath Path to output file (optional, derives from metadata)
     * @param password User password for key derivation
     * @param progressCallback Optional progress callback
     * @return Success or error
     */
    static Result<void> decryptFile(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::string& password,
        ProgressCallback progressCallback = nullptr);

    /**
     * Verify integrity of a .balance file
     * @param filePath Path to .balance file
     * @return true if integrity check passes, false otherwise
     */
    static Result<bool> verifyIntegrity(const std::filesystem::path& filePath);

    /**
     * Read metadata from a .balance file without decrypting
     * @param filePath Path to .balance file
     * @return File metadata or error
     */
    static Result<FileMetadata> readMetadata(const std::filesystem::path& filePath);

    /**
     * Derive AES key from password using PBKDF2
     * @param password User password
     * @param salt Salt for key derivation (16 bytes)
     * @param iterations Number of PBKDF2 iterations (default 100,000)
     * @return Derived key (32 bytes)
     */
    static Result<std::vector<uint8_t>> deriveKeyFromPassword(
        const std::string& password,
        const std::vector<uint8_t>& salt,
        int iterations = 100000);

private:
    /**
     * Apply XOR encryption to data
     * @param data Data to encrypt/decrypt
     * @param key XOR key (any length, will be repeated)
     * @return XOR'd data
     */
    static std::vector<uint8_t> xorEncrypt(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key);

    /**
     * Calculate SHA-256 hash of data
     * @param data Data to hash
     * @return SHA-256 hash (32 bytes)
     */
    static std::vector<uint8_t> calculateHash(const std::vector<uint8_t>& data);

    /**
     * Create Balance file header
     * @param originalSize Original file size
     * @param salt Salt for key derivation
     * @return Serialized header (64 bytes)
     */
    static std::vector<uint8_t> createHeader(
        uint64_t originalSize,
        const std::vector<uint8_t>& salt);

    /**
     * Parse Balance file header
     * @param headerData Header bytes (64 bytes)
     * @return Parsed header or error
     */
    static Result<BalanceHeader> parseHeader(const std::vector<uint8_t>& headerData);

    /**
     * Validate Balance file header
     * @param header Header to validate
     * @return true if valid, false otherwise
     */
    static bool validateHeader(const BalanceHeader& header);

    static constexpr size_t XOR_KEY_SIZE = 32;  // 256 bits
    static constexpr size_t RSA_ENCRYPTED_KEY_SIZE = 512;  // RSA-4096 output
    static constexpr size_t HASH_SIZE = 32;  // SHA-256
    static constexpr int PBKDF2_ITERATIONS = 100000;
};

} // namespace Balance
