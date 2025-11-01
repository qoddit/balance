#pragma once

#include "EncryptionEngine.h"
#include "balance/Types.h"

namespace Balance {

/**
 * AES-256-CBC encryption implementation
 * FIPS 197 compliant
 */
class AESEncryption : public EncryptionEngine {
public:
    AESEncryption();
    ~AESEncryption() override = default;

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
     * Generate a random IV (Initialization Vector)
     * @return 16-byte IV
     */
    static std::vector<uint8_t> generateIV();

    /**
     * Encrypt with explicit IV
     * @param data Data to encrypt
     * @param key Encryption key (must be 32 bytes)
     * @param iv Initialization vector (must be 16 bytes)
     * @return Encrypted data (IV prepended)
     */
    static Result<std::vector<uint8_t>> encryptWithIV(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv);

    /**
     * Decrypt with IV from prepended data
     * @param data Encrypted data (IV prepended)
     * @param key Decryption key (must be 32 bytes)
     * @return Decrypted data
     */
    static Result<std::vector<uint8_t>> decryptWithIV(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key);

private:
    static constexpr size_t KEY_SIZE = 32;  // 256 bits
    static constexpr size_t IV_SIZE = 16;   // 128 bits (AES block size)
};

} // namespace Balance
