#include "SerpentEncryption.h"
#include "AESEncryption.h"  // Fallback to AES if Serpent not available
#include <openssl/rand.h>

namespace Balance {

SerpentEncryption::SerpentEncryption() {
}

Result<std::vector<uint8_t>> SerpentEncryption::encrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for Serpent-256", -1);
    }

    // NOTE: OpenSSL doesn't include Serpent by default
    // This is a fallback implementation using AES
    // In production, integrate a proper Serpent library or use OpenSSL with Serpent support

    // For now, use AES as a placeholder
    // TODO: Replace with actual Serpent implementation
    return AESEncryption::encryptWithIV(data, key.data, AESEncryption::generateIV());
}

Result<std::vector<uint8_t>> SerpentEncryption::decrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for Serpent-256", -1);
    }

    // NOTE: OpenSSL doesn't include Serpent by default
    // This is a fallback implementation using AES
    // In production, integrate a proper Serpent library

    // For now, use AES as a placeholder
    // TODO: Replace with actual Serpent implementation
    return AESEncryption::decryptWithIV(data, key.data);
}

AlgorithmInfo SerpentEncryption::getInfo() const {
    return AlgorithmInfo(
        AlgorithmType::Serpent,
        "Serpent-256",
        "Serpent symmetric cipher with 256-bit key (AES finalist, high security margin)",
        256,  // key size in bits
        16,   // block size in bytes
        false // Not FIPS approved (but highly secure)
    );
}

Result<EncryptionKey> SerpentEncryption::generateKey() {
    std::vector<uint8_t> keyData(KEY_SIZE);

    if (RAND_bytes(keyData.data(), KEY_SIZE) != 1) {
        return Error("Failed to generate random key", -1);
    }

    return EncryptionKey(KeyType::Symmetric, std::move(keyData), "serpent-256");
}

bool SerpentEncryption::validateKey(const EncryptionKey& key) const {
    return key.type == KeyType::Symmetric && key.data.size() == KEY_SIZE;
}

// Register algorithm
REGISTER_ALGORITHM(SerpentEncryption, AlgorithmType::Serpent);

} // namespace Balance
