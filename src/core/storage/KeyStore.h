#pragma once

#include "balance/Types.h"
#include <filesystem>
#include <map>

namespace Balance {

/**
 * Abstract interface for key storage backends
 * Allows for software keystore or HSM integration
 */
class KeyStoreInterface {
public:
    virtual ~KeyStoreInterface() = default;

    virtual Result<void> saveKey(const std::string& identifier, const EncryptionKey& key) = 0;
    virtual Result<EncryptionKey> loadKey(const std::string& identifier) = 0;
    virtual Result<void> deleteKey(const std::string& identifier) = 0;
    virtual std::vector<std::string> listKeys() = 0;
};

/**
 * Software-based key storage (file-based)
 * Keys are encrypted with a master password
 */
class SoftwareKeyStore : public KeyStoreInterface {
public:
    explicit SoftwareKeyStore(const std::filesystem::path& storePath);
    ~SoftwareKeyStore() override = default;

    Result<void> saveKey(const std::string& identifier, const EncryptionKey& key) override;
    Result<EncryptionKey> loadKey(const std::string& identifier) override;
    Result<void> deleteKey(const std::string& identifier) override;
    std::vector<std::string> listKeys() override;

    /**
     * Initialize keystore with master password
     */
    Result<void> initialize(const std::string& masterPassword);

    /**
     * Check if keystore is initialized
     */
    bool isInitialized() const { return initialized; }

private:
    std::filesystem::path storePath;
    bool initialized;
    std::vector<uint8_t> masterKey;  // Derived from master password

    Result<std::filesystem::path> getKeyPath(const std::string& identifier);
    Result<std::vector<uint8_t>> encryptKeyData(const EncryptionKey& key);
    Result<EncryptionKey> decryptKeyData(const std::vector<uint8_t>& encryptedData);
};

/**
 * HSM-based key storage (PKCS#11 interface)
 * Phase 1: Stub implementation for future HSM integration
 */
class HSMKeyStore : public KeyStoreInterface {
public:
    HSMKeyStore() = default;
    ~HSMKeyStore() override = default;

    Result<void> saveKey(const std::string& identifier, const EncryptionKey& key) override;
    Result<EncryptionKey> loadKey(const std::string& identifier) override;
    Result<void> deleteKey(const std::string& identifier) override;
    std::vector<std::string> listKeys() override;

    /**
     * Connect to HSM
     * Phase 2: Implement PKCS#11 connection
     */
    Result<void> connect(const std::string& pkcs11Library, const std::string& pin);

private:
    bool connected = false;
    // Phase 2: Add PKCS#11 context
};

} // namespace Balance
