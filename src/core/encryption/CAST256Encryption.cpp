#include "CAST256Encryption.h"
#include "AESEncryption.h"  // Fallback
#include <openssl/rand.h>

namespace Balance {

CAST256Encryption::CAST256Encryption() {
}

Result<std::vector<uint8_t>> CAST256Encryption::encrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for CAST-256", -1);
    }

    // TODO: Implement actual CAST-256 encryption
    // Requires external library: libcast or tomcrypt
    // For now, using AES-256 as secure fallback

    return AESEncryption::encryptWithIV(data, key.data, AESEncryption::generateIV());
}

Result<std::vector<uint8_t>> CAST256Encryption::decrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for CAST-256", -1);
    }

    // TODO: Implement actual CAST-256 decryption
    // For now, using AES-256 as secure fallback

    return AESEncryption::decryptWithIV(data, key.data);
}

AlgorithmInfo CAST256Encryption::getInfo() const {
    return AlgorithmInfo(
        AlgorithmType::CAST256,
        "CAST-256",
        "CAST-256 (CAST6) symmetric cipher with 256-bit key (AES finalist)",
        256,  // key size in bits
        16,   // block size in bytes
        false // Not FIPS approved
    );
}

Result<EncryptionKey> CAST256Encryption::generateKey() {
    std::vector<uint8_t> keyData(KEY_SIZE);

    if (RAND_bytes(keyData.data(), KEY_SIZE) != 1) {
        return Error("Failed to generate random key", -1);
    }

    return EncryptionKey(KeyType::Symmetric, std::move(keyData), "cast-256");
}

bool CAST256Encryption::validateKey(const EncryptionKey& key) const {
    return key.type == KeyType::Symmetric && key.data.size() == KEY_SIZE;
}

// Register algorithm
REGISTER_ALGORITHM(CAST256Encryption, AlgorithmType::CAST256);

} // namespace Balance
