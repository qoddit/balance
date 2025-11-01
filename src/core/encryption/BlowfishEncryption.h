#pragma once

#include "EncryptionEngine.h"
#include "balance/Types.h"

namespace Balance {

/**
 * Blowfish-448 encryption implementation
 * Fast symmetric cipher, good for legacy compatibility
 */
class BlowfishEncryption : public EncryptionEngine {
public:
    BlowfishEncryption();
    ~BlowfishEncryption() override = default;

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
     * @return 8-byte IV (Blowfish block size)
     */
    static std::vector<uint8_t> generateIV();

private:
    static constexpr size_t KEY_SIZE = 56;  // 448 bits (maximum for Blowfish)
    static constexpr size_t IV_SIZE = 8;    // 64 bits (Blowfish block size)
};

} // namespace Balance
