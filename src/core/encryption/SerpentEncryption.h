#pragma once

#include "EncryptionEngine.h"
#include "balance/Types.h"

namespace Balance {

/**
 * Serpent-256 encryption implementation
 * AES finalist with high security margin
 * Note: OpenSSL may not include Serpent by default, this is a placeholder implementation
 */
class SerpentEncryption : public EncryptionEngine {
public:
    SerpentEncryption();
    ~SerpentEncryption() override = default;

    Result<std::vector<uint8_t>> encrypt(
        const std::vector<uint8_t>& data,
        const EncryptionKey& key) override;

    Result<std::vector<uint8_t>> decrypt(
        const std::vector<uint8_t>& data,
        const EncryptionKey& key) override;

    AlgorithmInfo getInfo() const override;

    Result<EncryptionKey> generateKey() override;

    bool validateKey(const EncryptionKey& key) const override;

private:
    static constexpr size_t KEY_SIZE = 32;  // 256 bits
    static constexpr size_t BLOCK_SIZE = 16;  // 128 bits
};

} // namespace Balance
