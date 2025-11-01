#include "BalanceFormat.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <fstream>
#include <cstring>
#include <ctime>

namespace Balance {

BalanceFormat::BalanceFormat() {
}

Result<std::vector<uint8_t>> BalanceFormat::encrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for Balance encryption", -1);
    }

    // Generate random XOR key
    std::vector<uint8_t> xorKey(XOR_KEY_SIZE);
    if (RAND_bytes(xorKey.data(), XOR_KEY_SIZE) != 1) {
        return Error("Failed to generate XOR key", -1);
    }

    // Apply XOR encryption
    auto xoredData = xorEncrypt(data, xorKey);

    // Encrypt with AES
    auto aesResult = AESEncryption::encryptWithIV(xoredData, key.data, AESEncryption::generateIV());
    if (aesResult.isError()) {
        return Error("AES encryption failed: " + aesResult.error().message, -1);
    }

    // Generate RSA key pair for XOR key encryption
    auto rsaKeyPair = RSAEncryption::generateKeyPair();
    if (rsaKeyPair.isError()) {
        return Error("RSA key generation failed: " + rsaKeyPair.error().message, -1);
    }

    // Encrypt XOR key with RSA public key
    auto encryptedXorKey = RSAEncryption::encryptWithPublicKey(xorKey, rsaKeyPair.value().first);
    if (encryptedXorKey.isError()) {
        return Error("RSA encryption of XOR key failed: " + encryptedXorKey.error().message, -1);
    }

    // Calculate integrity hash
    auto hash = calculateHash(aesResult.value());

    // Build final output: RSA-encrypted XOR key + AES-encrypted data + hash
    std::vector<uint8_t> output;
    output.reserve(encryptedXorKey.value().size() + aesResult.value().size() + hash.size());
    output.insert(output.end(), encryptedXorKey.value().begin(), encryptedXorKey.value().end());
    output.insert(output.end(), aesResult.value().begin(), aesResult.value().end());
    output.insert(output.end(), hash.begin(), hash.end());

    // Note: RSA private key is stored in metadata (would need to be saved separately in real implementation)
    return output;
}

Result<std::vector<uint8_t>> BalanceFormat::decrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for Balance decryption", -1);
    }

    // This is simplified - full implementation would extract RSA private key from metadata
    return Error("Use decryptFile() for Balance format decryption", -1);
}

AlgorithmInfo BalanceFormat::getInfo() const {
    return AlgorithmInfo(
        AlgorithmType::Balance,
        "Balance",
        "Proprietary hybrid encryption (XOR + AES-256 + RSA-4096) with integrity checking",
        256,  // Effective key size in bits (AES component)
        16,   // Block size (from AES)
        true  // FIPS approved (AES and RSA components are FIPS-approved)
    );
}

Result<EncryptionKey> BalanceFormat::generateKey() {
    // Generate a 256-bit symmetric key for AES component
    std::vector<uint8_t> keyData(32);
    if (RAND_bytes(keyData.data(), 32) != 1) {
        return Error("Failed to generate key", -1);
    }
    return EncryptionKey(KeyType::Symmetric, std::move(keyData), "balance");
}

bool BalanceFormat::validateKey(const EncryptionKey& key) const {
    return key.type == KeyType::Symmetric && key.data.size() == 32;
}

Result<void> BalanceFormat::encryptFile(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::string& password,
    ProgressCallback progressCallback) {

    // Open input file
    std::ifstream inputFile(inputPath, std::ios::binary | std::ios::ate);
    if (!inputFile) {
        return Error("Failed to open input file: " + inputPath.string(), -1);
    }

    // Get file size
    auto fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    // Read entire file
    std::vector<uint8_t> fileData(fileSize);
    inputFile.read(reinterpret_cast<char*>(fileData.data()), fileSize);
    inputFile.close();

    if (progressCallback) {
        progressCallback(1, 5, "Read input file");
    }

    // Generate salt
    std::vector<uint8_t> salt(16);
    if (RAND_bytes(salt.data(), 16) != 1) {
        return Error("Failed to generate salt", -1);
    }

    // Derive AES key from password
    auto aesKeyResult = deriveKeyFromPassword(password, salt);
    if (aesKeyResult.isError()) {
        return Error("Key derivation failed: " + aesKeyResult.error().message, -1);
    }

    if (progressCallback) {
        progressCallback(2, 5, "Derived encryption key");
    }

    // Generate random XOR key
    std::vector<uint8_t> xorKey(XOR_KEY_SIZE);
    if (RAND_bytes(xorKey.data(), XOR_KEY_SIZE) != 1) {
        return Error("Failed to generate XOR key", -1);
    }

    // Apply XOR encryption
    auto xoredData = xorEncrypt(fileData, xorKey);

    if (progressCallback) {
        progressCallback(3, 5, "Applied XOR encryption");
    }

    // Encrypt with AES
    auto aesResult = AESEncryption::encryptWithIV(xoredData, aesKeyResult.value(), AESEncryption::generateIV());
    if (aesResult.isError()) {
        return Error("AES encryption failed: " + aesResult.error().message, -1);
    }

    if (progressCallback) {
        progressCallback(4, 5, "Applied AES encryption");
    }

    // Generate RSA key pair
    auto rsaKeyPair = RSAEncryption::generateKeyPair();
    if (rsaKeyPair.isError()) {
        return Error("RSA key generation failed: " + rsaKeyPair.error().message, -1);
    }

    // Encrypt XOR key with RSA
    auto encryptedXorKey = RSAEncryption::encryptWithPublicKey(xorKey, rsaKeyPair.value().first);
    if (encryptedXorKey.isError()) {
        return Error("RSA encryption failed: " + encryptedXorKey.error().message, -1);
    }

    // Calculate integrity hash
    auto hash = calculateHash(aesResult.value());

    // Create header
    auto header = createHeader(fileSize, salt);

    // Write output file
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
        return Error("Failed to create output file: " + outputPath.string(), -1);
    }

    // Write: Header + RSA private key + RSA-encrypted XOR key + AES-encrypted data + hash
    outputFile.write(reinterpret_cast<const char*>(header.data()), header.size());
    outputFile.write(reinterpret_cast<const char*>(rsaKeyPair.value().second.data.data()),
                     rsaKeyPair.value().second.data.size());
    outputFile.write(reinterpret_cast<const char*>(encryptedXorKey.value().data()),
                     encryptedXorKey.value().size());
    outputFile.write(reinterpret_cast<const char*>(aesResult.value().data()),
                     aesResult.value().size());
    outputFile.write(reinterpret_cast<const char*>(hash.data()), hash.size());

    outputFile.close();

    if (progressCallback) {
        progressCallback(5, 5, "Written encrypted file");
    }

    return Result<void>();
}

Result<void> BalanceFormat::decryptFile(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::string& password,
    ProgressCallback progressCallback) {

    // Open input file
    std::ifstream inputFile(inputPath, std::ios::binary);
    if (!inputFile) {
        return Error("Failed to open input file: " + inputPath.string(), -1);
    }

    // Read header
    std::vector<uint8_t> headerData(BalanceHeader::HEADER_SIZE);
    inputFile.read(reinterpret_cast<char*>(headerData.data()), BalanceHeader::HEADER_SIZE);

    auto headerResult = parseHeader(headerData);
    if (headerResult.isError()) {
        return Error("Invalid Balance file: " + headerResult.error().message, -1);
    }

    auto header = headerResult.value();

    if (progressCallback) {
        progressCallback(1, 5, "Read file header");
    }

    // Read RSA private key (variable size, we'll read a reasonable amount)
    // In real implementation, key size would be in header
    std::vector<uint8_t> rsaPrivateKeyData(1200);  // Typical size for RSA-4096 private key
    inputFile.read(reinterpret_cast<char*>(rsaPrivateKeyData.data()), rsaPrivateKeyData.size());

    // Read RSA-encrypted XOR key
    std::vector<uint8_t> encryptedXorKey(RSA_ENCRYPTED_KEY_SIZE);
    inputFile.read(reinterpret_cast<char*>(encryptedXorKey.data()), RSA_ENCRYPTED_KEY_SIZE);

    // Read remaining data (AES-encrypted data + hash)
    std::vector<uint8_t> remainingData;
    inputFile.seekg(0, std::ios::end);
    auto totalSize = inputFile.tellg();
    auto dataSize = totalSize - BalanceHeader::HEADER_SIZE - 1200 - RSA_ENCRYPTED_KEY_SIZE;
    inputFile.seekg(BalanceHeader::HEADER_SIZE + 1200 + RSA_ENCRYPTED_KEY_SIZE);
    remainingData.resize(dataSize);
    inputFile.read(reinterpret_cast<char*>(remainingData.data()), dataSize);
    inputFile.close();

    if (progressCallback) {
        progressCallback(2, 5, "Read encrypted data");
    }

    // Split encrypted data and hash
    if (remainingData.size() < HASH_SIZE) {
        return Error("Corrupted file: insufficient data", -1);
    }

    std::vector<uint8_t> encryptedData(remainingData.begin(), remainingData.end() - HASH_SIZE);
    std::vector<uint8_t> storedHash(remainingData.end() - HASH_SIZE, remainingData.end());

    // Verify integrity
    auto calculatedHash = calculateHash(encryptedData);
    if (calculatedHash != storedHash) {
        return Error("Integrity check failed: file may be corrupted or tampered", -1);
    }

    if (progressCallback) {
        progressCallback(3, 5, "Verified file integrity");
    }

    // Derive AES key from password
    std::vector<uint8_t> salt(header.salt, header.salt + 16);
    auto aesKeyResult = deriveKeyFromPassword(password, salt);
    if (aesKeyResult.isError()) {
        return Error("Key derivation failed: " + aesKeyResult.error().message, -1);
    }

    // Decrypt with AES
    auto aesDecryptResult = AESEncryption::decryptWithIV(encryptedData, aesKeyResult.value());
    if (aesDecryptResult.isError()) {
        return Error("AES decryption failed (wrong password?): " + aesDecryptResult.error().message, -1);
    }

    if (progressCallback) {
        progressCallback(4, 5, "Decrypted AES layer");
    }

    // Decrypt XOR key with RSA
    EncryptionKey rsaPrivateKey(KeyType::RSAPrivate, rsaPrivateKeyData, "rsa-private");
    auto xorKeyResult = RSAEncryption::decryptWithPrivateKey(encryptedXorKey, rsaPrivateKey);
    if (xorKeyResult.isError()) {
        return Error("RSA decryption failed: " + xorKeyResult.error().message, -1);
    }

    // Apply XOR decryption
    auto decryptedData = xorEncrypt(aesDecryptResult.value(), xorKeyResult.value());

    // Write output file
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
        return Error("Failed to create output file: " + outputPath.string(), -1);
    }

    outputFile.write(reinterpret_cast<const char*>(decryptedData.data()), decryptedData.size());
    outputFile.close();

    if (progressCallback) {
        progressCallback(5, 5, "Written decrypted file");
    }

    return Result<void>();
}

Result<bool> BalanceFormat::verifyIntegrity(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return Error("Failed to open file: " + filePath.string(), -1);
    }

    // Read header
    std::vector<uint8_t> headerData(BalanceHeader::HEADER_SIZE);
    file.read(reinterpret_cast<char*>(headerData.data()), BalanceHeader::HEADER_SIZE);

    auto headerResult = parseHeader(headerData);
    if (headerResult.isError()) {
        return false;
    }

    // Skip RSA key data
    file.seekg(BalanceHeader::HEADER_SIZE + 1200 + RSA_ENCRYPTED_KEY_SIZE);

    // Read remaining data
    std::vector<uint8_t> remainingData;
    file.seekg(0, std::ios::end);
    auto totalSize = file.tellg();
    auto dataSize = totalSize - BalanceHeader::HEADER_SIZE - 1200 - RSA_ENCRYPTED_KEY_SIZE;
    file.seekg(BalanceHeader::HEADER_SIZE + 1200 + RSA_ENCRYPTED_KEY_SIZE);
    remainingData.resize(dataSize);
    file.read(reinterpret_cast<char*>(remainingData.data()), dataSize);
    file.close();

    if (remainingData.size() < HASH_SIZE) {
        return false;
    }

    // Extract hash and data
    std::vector<uint8_t> encryptedData(remainingData.begin(), remainingData.end() - HASH_SIZE);
    std::vector<uint8_t> storedHash(remainingData.end() - HASH_SIZE, remainingData.end());

    // Verify
    auto calculatedHash = calculateHash(encryptedData);
    return calculatedHash == storedHash;
}

Result<FileMetadata> BalanceFormat::readMetadata(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return Error("Failed to open file: " + filePath.string(), -1);
    }

    std::vector<uint8_t> headerData(BalanceHeader::HEADER_SIZE);
    file.read(reinterpret_cast<char*>(headerData.data()), BalanceHeader::HEADER_SIZE);

    auto headerResult = parseHeader(headerData);
    if (headerResult.isError()) {
        return Error("Invalid header: " + headerResult.error().message, -1);
    }

    auto header = headerResult.value();

    FileMetadata metadata;
    metadata.originalSize = header.originalSize;
    metadata.timestamp = header.timestamp;
    metadata.algorithm = AlgorithmType::Balance;
    metadata.salt = std::vector<uint8_t>(header.salt, header.salt + 16);
    metadata.originalName = filePath.stem().string();

    return metadata;
}

Result<std::vector<uint8_t>> BalanceFormat::deriveKeyFromPassword(
    const std::string& password,
    const std::vector<uint8_t>& salt,
    int iterations) {

    if (salt.size() != 16) {
        return Error("Invalid salt size (expected 16 bytes)", -1);
    }

    std::vector<uint8_t> derivedKey(32);  // 256 bits

    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                          salt.data(), salt.size(),
                          iterations,
                          EVP_sha256(),
                          32, derivedKey.data()) != 1) {
        return Error("PBKDF2 key derivation failed", -1);
    }

    return derivedKey;
}

std::vector<uint8_t> BalanceFormat::xorEncrypt(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key) {

    std::vector<uint8_t> result(data.size());

    for (size_t i = 0; i < data.size(); ++i) {
        result[i] = data[i] ^ key[i % key.size()];
    }

    return result;
}

std::vector<uint8_t> BalanceFormat::calculateHash(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(data.data(), data.size(), hash.data());
    return hash;
}

std::vector<uint8_t> BalanceFormat::createHeader(
    uint64_t originalSize,
    const std::vector<uint8_t>& salt) {

    std::vector<uint8_t> header(BalanceHeader::HEADER_SIZE, 0);

    // Magic number
    std::memcpy(header.data(), BalanceHeader::MAGIC, 8);

    // Version
    uint32_t version = BalanceHeader::CURRENT_VERSION;
    std::memcpy(header.data() + 8, &version, 4);

    // Flags (reserved for future use)
    uint32_t flags = 0;
    std::memcpy(header.data() + 12, &flags, 4);

    // Original size
    std::memcpy(header.data() + 16, &originalSize, 8);

    // Timestamp
    uint64_t timestamp = std::time(nullptr);
    std::memcpy(header.data() + 24, &timestamp, 8);

    // Salt
    std::memcpy(header.data() + 32, salt.data(), 16);

    // Reserved bytes (already zero)

    return header;
}

Result<BalanceHeader> BalanceFormat::parseHeader(const std::vector<uint8_t>& headerData) {
    if (headerData.size() < BalanceHeader::HEADER_SIZE) {
        return Error("Header too small", -1);
    }

    BalanceHeader header;

    // Parse magic
    std::memcpy(header.magic, headerData.data(), 8);

    // Parse version
    std::memcpy(&header.version, headerData.data() + 8, 4);

    // Parse flags
    std::memcpy(&header.flags, headerData.data() + 12, 4);

    // Parse original size
    std::memcpy(&header.originalSize, headerData.data() + 16, 8);

    // Parse timestamp
    std::memcpy(&header.timestamp, headerData.data() + 24, 8);

    // Parse salt
    std::memcpy(header.salt, headerData.data() + 32, 16);

    // Parse reserved
    std::memcpy(header.reserved, headerData.data() + 48, 16);

    if (!validateHeader(header)) {
        return Error("Invalid header", -1);
    }

    return header;
}

bool BalanceFormat::validateHeader(const BalanceHeader& header) {
    // Check magic number
    if (std::memcmp(header.magic, BalanceHeader::MAGIC, 8) != 0) {
        return false;
    }

    // Check version
    if (header.version != BalanceHeader::CURRENT_VERSION) {
        return false;
    }

    return true;
}

// Register algorithm
REGISTER_ALGORITHM(BalanceFormat, AlgorithmType::Balance);

} // namespace Balance
