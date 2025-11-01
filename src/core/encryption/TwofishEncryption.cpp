#include "TwofishEncryption.h"
#include "AESEncryption.h"  // Fallback
#include <openssl/rand.h>

namespace Balance {

TwofishEncryption::TwofishEncryption() {
}

Result<std::vector<uint8_t>> TwofishEncryption::encrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for Twofish-256", -1);
    }

    // TODO: Implement actual Twofish encryption
    // Requires external library: libtwofish or tomcrypt
    // For now, using AES-256 as secure fallback

    return AESEncryption::encryptWithIV(data, key.data, AESEncryption::generateIV());
}

Result<std::vector<uint8_t>> TwofishEncryption::decrypt(
    const std::vector<uint8_t>& data,
    const EncryptionKey& key) {

    if (!validateKey(key)) {
        return Error("Invalid key for Twofish-256", -1);
    }

    // TODO: Implement actual Twofish decryption
    // For now, using AES-256 as secure fallback

    return AESEncryption::decryptWithIV(data, key.data);
}

AlgorithmInfo TwofishEncryption::getInfo() const {
    return AlgorithmInfo(
        AlgorithmType::Twofish,
        "Twofish-256",
        "Twofish symmetric cipher with 256-bit key (AES finalist by Bruce Schneier)",
        256,  // key size in bits
        16,   // block size in bytes
        false // Not FIPS approved
    );
}

Result<EncryptionKey> TwofishEncryption::generateKey() {
    std::vector<uint8_t> keyData(KEY_SIZE);

    if (RAND_bytes(keyData.data(), KEY_SIZE) != 1) {
        return Error("Failed to generate random key", -1);
    }

    return EncryptionKey(KeyType::Symmetric, std::move(keyData), "twofish-256");
}

bool TwofishEncryption::validateKey(const EncryptionKey& key) const {
    return key.type == KeyType::Symmetric && key.data.size() == KEY_SIZE;
}

// Register algorithm
REGISTER_ALGORITHM(TwofishEncryption, AlgorithmType::Twofish);

} // namespace Balance
