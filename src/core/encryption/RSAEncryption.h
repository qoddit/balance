#pragma once

#include "EncryptionEngine.h"
#include "balance/Types.h"
#include <openssl/rsa.h>
#include <openssl/evp.h>

namespace Balance {

/**
 * RSA-4096 asymmetric encryption implementation
 * FIPS 186-4 compliant
 * Uses OAEP padding for secure encryption
 */
class RSAEncryption : public EncryptionEngine {
public:
    RSAEncryption();
    ~RSAEncryption() override = default;

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
     * Generate RSA key pair
     * @return Pair of (public key, private key)
     */
    static Result<std::pair<EncryptionKey, EncryptionKey>> generateKeyPair();

    /**
     * Encrypt data with public key
     * @param data Data to encrypt (max size depends on key size and padding)
     * @param publicKey RSA public key
     * @return Encrypted data
     */
    static Result<std::vector<uint8_t>> encryptWithPublicKey(
        const std::vector<uint8_t>& data,
        const EncryptionKey& publicKey);

    /**
     * Decrypt data with private key
     * @param data Encrypted data
     * @param privateKey RSA private key
     * @return Decrypted data
     */
    static Result<std::vector<uint8_t>> decryptWithPrivateKey(
        const std::vector<uint8_t>& data,
        const EncryptionKey& privateKey);

    /**
     * Get maximum data size that can be encrypted with RSA-4096
     * @return Maximum plaintext size in bytes
     */
    static size_t getMaxDataSize();

private:
    static constexpr int KEY_SIZE = 4096;  // 4096 bits
    static constexpr int KEY_SIZE_BYTES = KEY_SIZE / 8;  // 512 bytes
};

} // namespace Balance
