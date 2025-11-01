#include "KeyManager.h"
#include "../encryption/EncryptionEngine.h"
#include "../encryption/RSAEncryption.h"
#include "../encryption/BalanceFormat.h"
#include <openssl/rand.h>

namespace Balance {

KeyManager& KeyManager::instance() {
    static KeyManager manager;
    return manager;
}

Result<EncryptionKey> KeyManager::generateKey(AlgorithmType type, const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex);

    // Get engine for the algorithm
    auto engineResult = EncryptionRegistry::instance().createEngine(type);
    if (engineResult.isError()) {
        return Error("Failed to create engine: " + engineResult.error().message, -1);
    }

    // Generate key
    auto keyResult = engineResult.value()->generateKey();
    if (keyResult.isError()) {
        return keyResult;
    }

    auto& key = keyResult.value();

    // Set identifier if provided
    if (!identifier.empty()) {
        key.identifier = identifier;
    }

    // Store key if it has an identifier
    if (!key.identifier.empty()) {
        auto buffer = serializeKey(key);
        keys[key.identifier] = std::move(buffer);
        keyTypes[key.identifier] = key.type;
    }

    return key;
}

Result<std::pair<EncryptionKey, EncryptionKey>> KeyManager::generateRSAKeyPair(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex);

    auto keyPairResult = RSAEncryption::generateKeyPair();
    if (keyPairResult.isError()) {
        return Error(keyPairResult.error().message, keyPairResult.error().code);
    }

    auto& [publicKey, privateKey] = keyPairResult.value();

    // Set identifiers
    std::string pubId = identifier.empty() ? "rsa-public" : identifier + "-public";
    std::string privId = identifier.empty() ? "rsa-private" : identifier + "-private";

    publicKey.identifier = pubId;
    privateKey.identifier = privId;

    // Store keys
    auto pubBuffer = serializeKey(publicKey);
    auto privBuffer = serializeKey(privateKey);

    keys[pubId] = std::move(pubBuffer);
    keys[privId] = std::move(privBuffer);
    keyTypes[pubId] = KeyType::RSAPublic;
    keyTypes[privId] = KeyType::RSAPrivate;

    return std::make_pair(std::move(publicKey), std::move(privateKey));
}

Result<void> KeyManager::storeKey(const EncryptionKey& key, bool persistent) {
    std::lock_guard<std::mutex> lock(mutex);

    if (key.identifier.empty()) {
        return Error("Key must have an identifier to be stored", -1);
    }

    auto buffer = serializeKey(key);
    keys[key.identifier] = std::move(buffer);
    keyTypes[key.identifier] = key.type;

    // TODO: Implement persistent storage (write to encrypted keystore file)
    if (persistent) {
        // Placeholder for persistent storage
    }

    return Result<void>();
}

Result<EncryptionKey> KeyManager::getKey(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = keys.find(identifier);
    if (it == keys.end()) {
        return Error("Key not found: " + identifier, -1);
    }

    auto typeIt = keyTypes.find(identifier);
    if (typeIt == keyTypes.end()) {
        return Error("Key type information missing", -1);
    }

    return deserializeKey(*it->second, typeIt->second);
}

Result<void> KeyManager::removeKey(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = keys.find(identifier);
    if (it == keys.end()) {
        return Error("Key not found: " + identifier, -1);
    }

    keys.erase(it);
    keyTypes.erase(identifier);

    return Result<void>();
}

std::vector<std::string> KeyManager::listKeys() const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<std::string> identifiers;
    identifiers.reserve(keys.size());

    for (const auto& [id, _] : keys) {
        identifiers.push_back(id);
    }

    return identifiers;
}

void KeyManager::clearAll() {
    std::lock_guard<std::mutex> lock(mutex);
    keys.clear();
    keyTypes.clear();
}

Result<EncryptionKey> KeyManager::deriveKeyFromPassword(
    const std::string& password,
    const std::vector<uint8_t>& salt,
    AlgorithmType algorithm,
    int iterations) {

    auto keyResult = BalanceFormat::deriveKeyFromPassword(password, salt, iterations);
    if (keyResult.isError()) {
        return Error(keyResult.error().message, keyResult.error().code);
    }

    // Create key with appropriate identifier
    std::string algoName;
    switch (algorithm) {
        case AlgorithmType::Balance: algoName = "balance"; break;
        case AlgorithmType::AES256: algoName = "aes-256"; break;
        case AlgorithmType::Serpent: algoName = "serpent"; break;
        case AlgorithmType::Blowfish: algoName = "blowfish"; break;
        default: algoName = "derived"; break;
    }

    return EncryptionKey(KeyType::Symmetric, std::move(keyResult.value()), algoName + "-derived");
}

std::unique_ptr<SecureBuffer> KeyManager::serializeKey(const EncryptionKey& key) {
    // Simple serialization: just store the key data
    // In production, add versioning and metadata
    return std::make_unique<SecureBuffer>(key.data.data(), key.data.size());
}

Result<EncryptionKey> KeyManager::deserializeKey(const SecureBuffer& buffer, KeyType type) {
    std::vector<uint8_t> keyData(buffer.data(), buffer.data() + buffer.size());
    return EncryptionKey(type, std::move(keyData), "");
}

} // namespace Balance
