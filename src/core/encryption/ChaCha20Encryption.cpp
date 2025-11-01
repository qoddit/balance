#include "ChaCha20Encryption.h"
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace Balance {

ChaCha20Encryption::ChaCha20Encryption() {
}

Result<std::vector<uint8_t>> ChaCha20Encryption::encrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for ChaCha20-Poly1305", -1);
    }

    // Generate random nonce
    auto nonce = generateNonce();

    // Create cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Error("Failed to create cipher context", -1);
    }

    // Initialize encryption with ChaCha20-Poly1305
    if (EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, key.data.data(), nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Failed to initialize encryption", -1);
    }

    // Allocate output buffer (nonce + ciphertext + tag)
    std::vector<uint8_t> output;
    output.reserve(NONCE_SIZE + data.size() + TAG_SIZE);

    // Prepend nonce
    output.insert(output.end(), nonce.begin(), nonce.end());

    // Encrypt data
    std::vector<uint8_t> ciphertext(data.size() + EVP_CIPHER_block_size(EVP_chacha20_poly1305()));
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

    // Get authentication tag
    std::vector<uint8_t> tag(TAG_SIZE);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, TAG_SIZE, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Failed to get authentication tag", -1);
    }

    // Append tag to output
    output.insert(output.end(), tag.begin(), tag.end());

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return output;
}

Result<std::vector<uint8_t>> ChaCha20Encryption::decrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for ChaCha20-Poly1305", -1);
    }

    if (data.size() < NONCE_SIZE + TAG_SIZE) {
        return Error("Invalid encrypted data (too small)", -1);
    }

    // Extract nonce
    std::vector<uint8_t> nonce(data.begin(), data.begin() + NONCE_SIZE);

    // Extract ciphertext (everything between nonce and tag)
    size_t ciphertext_len = data.size() - NONCE_SIZE - TAG_SIZE;
    std::vector<uint8_t> ciphertext(data.begin() + NONCE_SIZE, data.begin() + NONCE_SIZE + ciphertext_len);

    // Extract tag
    std::vector<uint8_t> tag(data.end() - TAG_SIZE, data.end());

    // Create cipher context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Error("Failed to create cipher context", -1);
    }

    // Initialize decryption
    if (EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, key.data.data(), nonce.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Failed to initialize decryption", -1);
    }

    // Decrypt data
    std::vector<uint8_t> plaintext(ciphertext.size() + EVP_CIPHER_block_size(EVP_chacha20_poly1305()));
    int len = 0;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Decryption failed", -1);
    }

    int plaintext_len = len;

    // Set authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, TAG_SIZE, tag.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Failed to set authentication tag", -1);
    }

    // Finalize decryption (verifies authentication tag)
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Error("Decryption finalization failed (authentication failed)", -1);
    }

    plaintext_len += len;
    plaintext.resize(plaintext_len);

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return plaintext;
}

AlgorithmInfo ChaCha20Encryption::getInfo() const {
    return AlgorithmInfo(
        AlgorithmType::ChaCha20,
        "ChaCha20-Poly1305",
        "Modern AEAD cipher with authenticated encryption (fast, secure, constant-time)",
        256,  // key size in bits
        64,   // ChaCha20 block size in bytes
        false // Not FIPS approved (but widely trusted and used)
    );
}

Result<EncryptionKey> ChaCha20Encryption::generateKey() {
    std::vector<uint8_t> keyData(KEY_SIZE);

    if (RAND_bytes(keyData.data(), KEY_SIZE) != 1) {
        return Error("Failed to generate random key", -1);
    }

    return EncryptionKey(KeyType::Symmetric, std::move(keyData), "chacha20-poly1305");
}

bool ChaCha20Encryption::validateKey(const EncryptionKey& key) const {
    return key.type == KeyType::Symmetric && key.data.size() == KEY_SIZE;
}

std::vector<uint8_t> ChaCha20Encryption::generateNonce() {
    std::vector<uint8_t> nonce(NONCE_SIZE);
    RAND_bytes(nonce.data(), NONCE_SIZE);
    return nonce;
}

// Register algorithm
REGISTER_ALGORITHM(ChaCha20Encryption, AlgorithmType::ChaCha20);

} // namespace Balance
