#pragma once

#include "EncryptionEngine.h"
#include "balance/Types.h"

namespace Balance {

/**
 * ChaCha20-Poly1305 AEAD encryption implementation
 * Modern, fast, and secure authenticated encryption
 */
class ChaCha20Encryption : public EncryptionEngine {
public:
    ChaCha20Encryption();
    ~ChaCha20Encryption() override = default;

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
     * Generate a random nonce
     * @return 12-byte nonce for ChaCha20-Poly1305
     */
    static std::vector<uint8_t> generateNonce();

private:
    static constexpr size_t KEY_SIZE = 32;    // 256 bits
    static constexpr size_t NONCE_SIZE = 12;  // 96 bits
    static constexpr size_t TAG_SIZE = 16;    // 128 bits (Poly1305 MAC)
};

} // namespace Balance
