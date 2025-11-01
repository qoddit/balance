#pragma once

#include "balance/Types.h"
#include "SecureMemory.h"
#include <map>
#include <mutex>
#include <memory>

namespace Balance {

/**
 * Manages encryption keys
 * Provides key generation, storage, retrieval, and lifecycle management
 */
class KeyManager {
public:
    /**
     * Get singleton instance
     */
    static KeyManager& instance();

    /**
     * Generate a new encryption key
     * @param type Algorithm type for the key
     * @param identifier Optional key identifier
     * @return Generated key or error
     */
    Result<EncryptionKey> generateKey(AlgorithmType type, const std::string& identifier = "");

    /**
     * Generate RSA key pair
     * @param identifier Optional key identifier prefix
     * @return Pair of (public key, private key)
     */
    Result<std::pair<EncryptionKey, EncryptionKey>> generateRSAKeyPair(const std::string& identifier = "");

    /**
     * Store a key
     * @param key Key to store
     * @param persistent If true, save to disk
     * @return Success or error
     */
    Result<void> storeKey(const EncryptionKey& key, bool persistent = false);

    /**
     * Retrieve a key by identifier
     * @param identifier Key identifier
     * @return Key or error
     */
    Result<EncryptionKey> getKey(const std::string& identifier);

    /**
     * Remove a key
     * @param identifier Key identifier
     * @return Success or error
     */
    Result<void> removeKey(const std::string& identifier);

    /**
     * List all stored key identifiers
     * @return Vector of key identifiers
     */
    std::vector<std::string> listKeys() const;

    /**
     * Clear all keys from memory
     */
    void clearAll();

    /**
     * Derive key from password
     * @param password User password
     * @param salt Salt for key derivation (16 bytes)
     * @param algorithm Target algorithm type
     * @param iterations PBKDF2 iterations
     * @return Derived key or error
     */
    Result<EncryptionKey> deriveKeyFromPassword(
        const std::string& password,
        const std::vector<uint8_t>& salt,
        AlgorithmType algorithm,
        int iterations = 100000);

    // Delete copy constructor and assignment
    KeyManager(const KeyManager&) = delete;
    KeyManager& operator=(const KeyManager&) = delete;

private:
    KeyManager() = default;

    std::map<std::string, std::unique_ptr<SecureBuffer>> keys;
    std::map<std::string, KeyType> keyTypes;
    mutable std::mutex mutex;

    /**
     * Serialize key to SecureBuffer
     */
    std::unique_ptr<SecureBuffer> serializeKey(const EncryptionKey& key);

    /**
     * Deserialize key from SecureBuffer
     */
    Result<EncryptionKey> deserializeKey(const SecureBuffer& buffer, KeyType type);
};

} // namespace Balance
