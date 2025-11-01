#include "BlowfishEncryption.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/blowfish.h>

namespace Balance {

BlowfishEncryption::BlowfishEncryption() {
}

Result<std::vector<uint8_t>> BlowfishEncryption::encrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for Blowfish", -1);
    }

    // Generate random IV
    auto iv = generateIV();

    // Create and initialize context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Error("Failed to create cipher context", -1);
    }

    // Initialize encryption with Blowfish-CBC
    if (EVP_EncryptInit_ex(ctx, EVP_bf_cbc(), nullptr, key.data.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Failed to initialize encryption", -1);
    }

    // Allocate output buffer
    std::vector<uint8_t> output;
    output.reserve(IV_SIZE + data.size() + BF_BLOCK);

    // Prepend IV to output
    output.insert(output.end(), iv.begin(), iv.end());

    // Encrypt data
    std::vector<uint8_t> ciphertext(data.size() + BF_BLOCK);
    int len = 0;

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, data.data(), static_cast<int>(data.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Encryption failed", -1);
    }

    int ciphertext_len = len;

    // Finalize encryption
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

Result<std::vector<uint8_t>> BlowfishEncryption::decrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for Blowfish", -1);
    }

    if (data.size() < IV_SIZE) {
        return Error("Invalid encrypted data (too small)", -1);
    }

    // Extract IV from beginning of data
    std::vector<uint8_t> iv(data.begin(), data.begin() + IV_SIZE);

    // Extract ciphertext
    std::vector<uint8_t> ciphertext(data.begin() + IV_SIZE, data.end());

    // Create and initialize context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Error("Failed to create cipher context", -1);
    }

    // Initialize decryption
    if (EVP_DecryptInit_ex(ctx, EVP_bf_cbc(), nullptr, key.data.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Failed to initialize decryption", -1);
    }

    // Decrypt data
    std::vector<uint8_t> plaintext(ciphertext.size() + BF_BLOCK);
    int len = 0;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Decryption failed", -1);
    }

    int plaintext_len = len;

    // Finalize decryption
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

AlgorithmInfo BlowfishEncryption::getInfo() const {
    return AlgorithmInfo(
        AlgorithmType::Blowfish,
        "Blowfish-448",
        "Blowfish symmetric cipher with 448-bit key (fast, legacy compatibility)",
        448,  // key size in bits
        8,    // block size in bytes
        false // Not FIPS approved
    );
}

Result<EncryptionKey> BlowfishEncryption::generateKey() {
    std::vector<uint8_t> keyData(KEY_SIZE);

    if (RAND_bytes(keyData.data(), KEY_SIZE) != 1) {
        return Error("Failed to generate random key", -1);
    }

    return EncryptionKey(KeyType::Symmetric, std::move(keyData), "blowfish-448");
}

bool BlowfishEncryption::validateKey(const EncryptionKey& key) const {
    // Blowfish supports keys from 32 to 448 bits
    return key.type == KeyType::Symmetric && key.data.size() >= 4 && key.data.size() <= KEY_SIZE;
}

std::vector<uint8_t> BlowfishEncryption::generateIV() {
    std::vector<uint8_t> iv(IV_SIZE);
    RAND_bytes(iv.data(), IV_SIZE);
    return iv;
}

// Register algorithm
REGISTER_ALGORITHM(BlowfishEncryption, AlgorithmType::Blowfish);

} // namespace Balance
