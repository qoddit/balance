#include "RSAEncryption.h"
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <cstring>

namespace Balance {

RSAEncryption::RSAEncryption() {
}

Result<std::vector<uint8_t>> RSAEncryption::encrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (key.type != KeyType::RSAPublic) {
        return Error("RSA encryption requires a public key", -1);
    }

    return encryptWithPublicKey(data, key);
}

Result<std::vector<uint8_t>> RSAEncryption::decrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (key.type != KeyType::RSAPrivate) {
        return Error("RSA decryption requires a private key", -1);
    }

    return decryptWithPrivateKey(data, key);
}

AlgorithmInfo RSAEncryption::getInfo() const {
    return AlgorithmInfo(
        AlgorithmType::RSA4096,
        "RSA-4096",
        "RSA asymmetric encryption with 4096-bit keys and OAEP padding (FIPS 186-4)",
        4096,  // key size in bits
        512,   // RSA doesn't use blocks like symmetric ciphers, but this is the key size in bytes
        true   // FIPS approved
    );
}

Result<EncryptionKey> RSAEncryption::generateKey() {
    // For RSA, we return the public key by default
    // Use generateKeyPair() to get both keys
    auto result = generateKeyPair();
    if (result.isError()) {
        return Error(result.error().message, result.error().code);
    }

    return std::move(result.value().first);
}

bool RSAEncryption::validateKey(const EncryptionKey& key) const {
    return (key.type == KeyType::RSAPublic || key.type == KeyType::RSAPrivate) &&
           !key.data.empty();
}

Result<std::pair<EncryptionKey, EncryptionKey>> RSAEncryption::generateKeyPair() {
    // Generate RSA key
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        return Error("Failed to create key generation context", -1);
    }

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return Error("Failed to initialize key generation", -1);
    }

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, KEY_SIZE) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return Error("Failed to set key size", -1);
    }

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return Error("Failed to generate RSA key pair", -1);
    }

    EVP_PKEY_CTX_free(ctx);

    // Export public key to DER format
    unsigned char* pubDer = nullptr;
    int pubLen = i2d_PUBKEY(pkey, &pubDer);
    if (pubLen < 0) {
        EVP_PKEY_free(pkey);
        return Error("Failed to export public key", -1);
    }

    std::vector<uint8_t> publicKeyData(pubDer, pubDer + pubLen);
    OPENSSL_free(pubDer);

    // Export private key to DER format
    unsigned char* privDer = nullptr;
    int privLen = i2d_PrivateKey(pkey, &privDer);
    if (privLen < 0) {
        EVP_PKEY_free(pkey);
        return Error("Failed to export private key", -1);
    }

    std::vector<uint8_t> privateKeyData(privDer, privDer + privLen);
    OPENSSL_free(privDer);

    EVP_PKEY_free(pkey);

    EncryptionKey publicKey(KeyType::RSAPublic, std::move(publicKeyData), "rsa-4096-public");
    EncryptionKey privateKey(KeyType::RSAPrivate, std::move(privateKeyData), "rsa-4096-private");

    return std::make_pair(std::move(publicKey), std::move(privateKey));
}

Result<std::vector<uint8_t>> RSAEncryption::encryptWithPublicKey(
    const std::vector<uint8_t>& data,
    const EncryptionKey& publicKey) {

    if (publicKey.type != KeyType::RSAPublic) {
        return Error("Invalid key type (expected RSA public key)", -1);
    }

    // Check data size
    if (data.size() > getMaxDataSize()) {
        return Error("Data too large for RSA encryption (max " +
                     std::to_string(getMaxDataSize()) + " bytes)", -1);
    }

    // Load public key from DER format
    const unsigned char* keyData = publicKey.data.data();
    EVP_PKEY* pkey = d2i_PUBKEY(nullptr, &keyData, static_cast<long>(publicKey.data.size()));
    if (!pkey) {
        return Error("Failed to load public key", -1);
    }

    // Create encryption context
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return Error("Failed to create encryption context", -1);
    }

    // Initialize encryption
    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return Error("Failed to initialize encryption", -1);
    }

    // Set padding to OAEP (more secure than PKCS#1 v1.5)
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return Error("Failed to set padding mode", -1);
    }

    // Determine output buffer size
    size_t outlen;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, data.data(), data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return Error("Failed to determine output size", -1);
    }

    // Encrypt
    std::vector<uint8_t> encrypted(outlen);
    if (EVP_PKEY_encrypt(ctx, encrypted.data(), &outlen, data.data(), data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return Error("Encryption failed", -1);
    }

    encrypted.resize(outlen);

    // Clean up
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return encrypted;
}

Result<std::vector<uint8_t>> RSAEncryption::decryptWithPrivateKey(
    const std::vector<uint8_t>& data,
    const EncryptionKey& privateKey) {

    if (privateKey.type != KeyType::RSAPrivate) {
        return Error("Invalid key type (expected RSA private key)", -1);
    }

    // Load private key from DER format
    const unsigned char* keyData = privateKey.data.data();
    EVP_PKEY* pkey = d2i_PrivateKey(EVP_PKEY_RSA, nullptr, &keyData, static_cast<long>(privateKey.data.size()));
    if (!pkey) {
        return Error("Failed to load private key", -1);
    }

    // Create decryption context
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return Error("Failed to create decryption context", -1);
    }

    // Initialize decryption
    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return Error("Failed to initialize decryption", -1);
    }

    // Set padding to OAEP
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return Error("Failed to set padding mode", -1);
    }

    // Determine output buffer size
    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, data.data(), data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return Error("Failed to determine output size", -1);
    }

    // Decrypt
    std::vector<uint8_t> decrypted(outlen);
    if (EVP_PKEY_decrypt(ctx, decrypted.data(), &outlen, data.data(), data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return Error("Decryption failed (wrong key or corrupted data)", -1);
    }

    decrypted.resize(outlen);

    // Clean up
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return decrypted;
}

size_t RSAEncryption::getMaxDataSize() {
    // With OAEP padding and SHA-256, the maximum data size is:
    // key_size_bytes - 2 * hash_size - 2
    // For RSA-4096 with SHA-256: 512 - 2*32 - 2 = 446 bytes
    return KEY_SIZE_BYTES - 2 * 32 - 2;
}

// Register algorithm
REGISTER_ALGORITHM(RSAEncryption, AlgorithmType::RSA4096);

} // namespace Balance
