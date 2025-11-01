#include "AESEncryption.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <cstring>

namespace Balance {

AESEncryption::AESEncryption() {
}

Result<std::vector<uint8_t>> AESEncryption::encrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for AES-256", -1);
    }

    // Generate random IV
    auto iv = generateIV();

    // Encrypt with the IV
    return encryptWithIV(data, key.data, iv);
}

Result<std::vector<uint8_t>> AESEncryption::decrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for AES-256", -1);
    }

    return decryptWithIV(data, key.data);
}

AlgorithmInfo AESEncryption::getInfo() const {
    return AlgorithmInfo(
        AlgorithmType::AES256,
        "AES-256-CBC",
        "Advanced Encryption Standard with 256-bit key in CBC mode (FIPS 197)",
        256,  // key size in bits
        16,   // block size in bytes
        true  // FIPS approved
    );
}

Result<EncryptionKey> AESEncryption::generateKey() {
    std::vector<uint8_t> keyData(KEY_SIZE);

    if (RAND_bytes(keyData.data(), KEY_SIZE) != 1) {
        return Error("Failed to generate random key", -1);
    }

    return EncryptionKey(KeyType::Symmetric, std::move(keyData), "aes-256");
}

bool AESEncryption::validateKey(const EncryptionKey& key) const {
    return key.type == KeyType::Symmetric && key.data.size() == KEY_SIZE;
}

std::vector<uint8_t> AESEncryption::generateIV() {
    std::vector<uint8_t> iv(IV_SIZE);
    RAND_bytes(iv.data(), IV_SIZE);
    return iv;
}

Result<std::vector<uint8_t>> AESEncryption::encryptWithIV(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv) {

    if (key.size() != KEY_SIZE) {
        return Error("Invalid key size for AES-256 (expected 32 bytes)", -1);
    }

    if (iv.size() != IV_SIZE) {
        return Error("Invalid IV size (expected 16 bytes)", -1);
    }

    // Create and initialize context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Error("Failed to create cipher context", -1);
    }

    // Initialize encryption operation
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Failed to initialize encryption", -1);
    }

    // Allocate output buffer (input size + block size for padding + IV size)
    std::vector<uint8_t> output;
    output.reserve(IV_SIZE + data.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));

    // Prepend IV to output
    output.insert(output.end(), iv.begin(), iv.end());

    // Encrypt data
    std::vector<uint8_t> ciphertext(data.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    int len = 0;

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, data.data(), static_cast<int>(data.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Encryption failed", -1);
    }

    int ciphertext_len = len;

    // Finalize encryption (handles padding)
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Encryption finalization failed", -1);
    }

    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);

    // Append ciphertext to output
    output.insert(output.end(), ciphertext.begin(), ciphertext.end());

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return output;
}

Result<std::vector<uint8_t>> AESEncryption::decryptWithIV(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key) {

    if (key.size() != KEY_SIZE) {
        return Error("Invalid key size for AES-256 (expected 32 bytes)", -1);
    }

    if (data.size() < IV_SIZE) {
        return Error("Invalid encrypted data (too small)", -1);
    }

    // Extract IV from beginning of data
    std::vector<uint8_t> iv(data.begin(), data.begin() + IV_SIZE);

    // Extract ciphertext (everything after IV)
    std::vector<uint8_t> ciphertext(data.begin() + IV_SIZE, data.end());

    // Create and initialize context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Error("Failed to create cipher context", -1);
    }

    // Initialize decryption operation
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Failed to initialize decryption", -1);
    }

    // Decrypt data
    std::vector<uint8_t> plaintext(ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    int len = 0;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Decryption failed", -1);
    }

    int plaintext_len = len;

    // Finalize decryption (handles padding removal)
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Decryption finalization failed (wrong key or corrupted data)", -1);
    }

    plaintext_len += len;
    plaintext.resize(plaintext_len);

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return plaintext;
}

// Register algorithm
REGISTER_ALGORITHM(AESEncryption, AlgorithmType::AES256);

} // namespace Balance
