#pragma once

#include "EncryptionEngine.h"
#include "balance/Types.h"

namespace Balance {

/**
 * CAST-256 (CAST6) encryption implementation
 * AES finalist, extension of CAST-128
 * Note: Requires external CAST library or fallback to AES
 */
class CAST256Encryption : public EncryptionEngine {
public:
    CAST256Encryption();
    ~CAST256Encryption() override = default;

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
