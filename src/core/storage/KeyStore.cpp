#include "KeyStore.h"
#include "../encryption/AESEncryption.h"
#include "../encryption/BalanceFormat.h"
#include <fstream>
#include <openssl/rand.h>

namespace Balance {

// SoftwareKeyStore implementation

SoftwareKeyStore::SoftwareKeyStore(const std::filesystem::path& storePath)
    : storePath(storePath), initialized(false) {

    // Create directory if it doesn't exist
    if (!std::filesystem::exists(storePath)) {
        std::filesystem::create_directories(storePath);
    }
}

Result<void> SoftwareKeyStore::initialize(const std::string& masterPassword) {
    // Generate salt
    std::vector<uint8_t> salt(16);
    if (RAND_bytes(salt.data(), 16) != 1) {
        return Error("Failed to generate salt", -1);
    }

    // Derive master key from password
    auto keyResult = BalanceFormat::deriveKeyFromPassword(masterPassword, salt, 100000);
    if (keyResult.isError()) {
        return Error("Failed to derive master key: " + keyResult.error().message, -1);
    }

    masterKey = std::move(keyResult.value());
    initialized = true;

    // Save salt to file
    auto saltPath = storePath / ".salt";
    std::ofstream saltFile(saltPath, std::ios::binary);
    if (!saltFile) {
        return Error("Failed to save salt file", -1);
    }
    saltFile.write(reinterpret_cast<const char*>(salt.data()), salt.size());
    saltFile.close();

    return Result<void>();
}

Result<void> SoftwareKeyStore::saveKey(const std::string& identifier, const EncryptionKey& key) {
    if (!initialized) {
        return Error("KeyStore not initialized", -1);
    }

    auto encryptedResult = encryptKeyData(key);
    if (encryptedResult.isError()) {
        return Error(encryptedResult.error().message, encryptedResult.error().code);
    }

    auto pathResult = getKeyPath(identifier);
    if (pathResult.isError()) {
        return Error(pathResult.error().message, pathResult.error().code);
    }

    std::ofstream file(pathResult.value(), std::ios::binary);
    if (!file) {
        return Error("Failed to open key file for writing", -1);
    }

    auto& encrypted = encryptedResult.value();
    file.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
    file.close();

    return Result<void>();
}

Result<EncryptionKey> SoftwareKeyStore::loadKey(const std::string& identifier) {
    if (!initialized) {
        return Error("KeyStore not initialized", -1);
    }

    auto pathResult = getKeyPath(identifier);
    if (pathResult.isError()) {
        return Error(pathResult.error().message, pathResult.error().code);
    }

    std::ifstream file(pathResult.value(), std::ios::binary | std::ios::ate);
    if (!file) {
        return Error("Key not found: " + identifier, -1);
    }

    auto fileSize = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> encryptedData(fileSize);
    file.read(reinterpret_cast<char*>(encryptedData.data()), fileSize);
    file.close();

    return decryptKeyData(encryptedData);
}

Result<void> SoftwareKeyStore::deleteKey(const std::string& identifier) {
    auto pathResult = getKeyPath(identifier);
    if (pathResult.isError()) {
        return Error(pathResult.error().message, pathResult.error().code);
    }

    if (!std::filesystem::exists(pathResult.value())) {
        return Error("Key not found: " + identifier, -1);
    }

    std::filesystem::remove(pathResult.value());
    return Result<void>();
}

std::vector<std::string> SoftwareKeyStore::listKeys() {
    std::vector<std::string> keys;

    if (!std::filesystem::exists(storePath)) {
        return keys;
    }

    for (const auto& entry : std::filesystem::directory_iterator(storePath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename != ".salt" && filename.ends_with(".key")) {
                keys.push_back(filename.substr(0, filename.length() - 4));
            }
        }
    }

    return keys;
}

Result<std::filesystem::path> SoftwareKeyStore::getKeyPath(const std::string& identifier) {
    // Sanitize identifier
    std::string sanitized = identifier;
    for (char& c : sanitized) {
        if (!std::isalnum(c) && c != '-' && c != '_') {
            c = '_';
        }
    }

    return storePath / (sanitized + ".key");
}

Result<std::vector<uint8_t>> SoftwareKeyStore::encryptKeyData(const EncryptionKey& key) {
    // Serialize key: [type:1][identifier_len:2][identifier:n][data_len:4][data:n]
    std::vector<uint8_t> serialized;

    // Key type
    serialized.push_back(static_cast<uint8_t>(key.type));

    // Identifier
    uint16_t idLen = static_cast<uint16_t>(key.identifier.length());
    serialized.push_back(idLen & 0xFF);
    serialized.push_back((idLen >> 8) & 0xFF);
    serialized.insert(serialized.end(), key.identifier.begin(), key.identifier.end());

    // Key data
    uint32_t dataLen = static_cast<uint32_t>(key.data.size());
    serialized.push_back(dataLen & 0xFF);
    serialized.push_back((dataLen >> 8) & 0xFF);
    serialized.push_back((dataLen >> 16) & 0xFF);
    serialized.push_back((dataLen >> 24) & 0xFF);
    serialized.insert(serialized.end(), key.data.begin(), key.data.end());

    // Encrypt with master key
    return AESEncryption::encryptWithIV(serialized, masterKey, AESEncryption::generateIV());
}

Result<EncryptionKey> SoftwareKeyStore::decryptKeyData(const std::vector<uint8_t>& encryptedData) {
    // Decrypt
    auto decryptResult = AESEncryption::decryptWithIV(encryptedData, masterKey);
    if (decryptResult.isError()) {
        return Error("Decryption failed: " + decryptResult.error().message, -1);
    }

    auto& data = decryptResult.value();

    if (data.size() < 7) {
        return Error("Invalid key data", -1);
    }

    size_t pos = 0;

    // Parse type
    KeyType type = static_cast<KeyType>(data[pos++]);

    // Parse identifier
    uint16_t idLen = data[pos] | (data[pos + 1] << 8);
    pos += 2;

    if (pos + idLen > data.size()) {
        return Error("Invalid key data: identifier overflow", -1);
    }

    std::string identifier(data.begin() + pos, data.begin() + pos + idLen);
    pos += idLen;

    // Parse key data
    if (pos + 4 > data.size()) {
        return Error("Invalid key data: length overflow", -1);
    }

    uint32_t dataLen = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
    pos += 4;

    if (pos + dataLen > data.size()) {
        return Error("Invalid key data: data overflow", -1);
    }

    std::vector<uint8_t> keyData(data.begin() + pos, data.begin() + pos + dataLen);

    return EncryptionKey(type, std::move(keyData), identifier);
}

// HSMKeyStore stub implementation

Result<void> HSMKeyStore::saveKey(const std::string& identifier, const EncryptionKey& key) {
    // TODO: Phase 2 - Implement PKCS#11 key storage
    return Error("HSM support not yet implemented", -1);
}

Result<EncryptionKey> HSMKeyStore::loadKey(const std::string& identifier) {
    // TODO: Phase 2 - Implement PKCS#11 key retrieval
    return Error("HSM support not yet implemented", -1);
}

Result<void> HSMKeyStore::deleteKey(const std::string& identifier) {
    // TODO: Phase 2 - Implement PKCS#11 key deletion
    return Error("HSM support not yet implemented", -1);
}

std::vector<std::string> HSMKeyStore::listKeys() {
    // TODO: Phase 2 - Implement PKCS#11 key listing
    return {};
}

Result<void> HSMKeyStore::connect(const std::string& pkcs11Library, const std::string& pin) {
    // TODO: Phase 2 - Implement PKCS#11 connection
    return Error("HSM support not yet implemented", -1);
}

} // namespace Balance
