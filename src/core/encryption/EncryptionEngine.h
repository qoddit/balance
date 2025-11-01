#pragma once

#include "balance/Types.h"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <functional>

namespace Balance {

/**
 * Abstract base class for all encryption algorithms
 * Provides interface for encryption/decryption operations
 */
class EncryptionEngine {
public:
    virtual ~EncryptionEngine() = default;

    /**
     * Encrypt data with the given key
     * @param data Input data to encrypt
     * @param key Encryption key
     * @return Encrypted data or error
     */
    virtual Result<std::vector<uint8_t>> encrypt(
        const std::vector<uint8_t>& data,
        const EncryptionKey& key) = 0;

    /**
     * Decrypt data with the given key
     * @param data Encrypted data
     * @param key Decryption key
     * @return Decrypted data or error
     */
    virtual Result<std::vector<uint8_t>> decrypt(
        const std::vector<uint8_t>& data,
        const EncryptionKey& key) = 0;

    /**
     * Get algorithm information
     * @return Algorithm metadata
     */
    virtual AlgorithmInfo getInfo() const = 0;

    /**
     * Generate a new key for this algorithm
     * @return New encryption key or error
     */
    virtual Result<EncryptionKey> generateKey() = 0;

    /**
     * Validate key for this algorithm
     * @param key Key to validate
     * @return true if valid, false otherwise
     */
    virtual bool validateKey(const EncryptionKey& key) const = 0;
};

/**
 * Factory and registry for encryption algorithms
 * Thread-safe singleton pattern
 */
class EncryptionRegistry {
public:
    using EngineCreator = std::function<std::unique_ptr<EncryptionEngine>()>;

    /**
     * Get singleton instance
     */
    static EncryptionRegistry& instance();

    /**
     * Register an encryption algorithm
     * @param type Algorithm type
     * @param creator Factory function to create algorithm instance
     */
    void registerAlgorithm(AlgorithmType type, EngineCreator creator);

    /**
     * Create an encryption engine instance
     * @param type Algorithm type
     * @return Encryption engine instance or error
     */
    Result<std::unique_ptr<EncryptionEngine>> createEngine(AlgorithmType type);

    /**
     * Get information about all supported algorithms
     * @return List of algorithm information
     */
    std::vector<AlgorithmInfo> getSupportedAlgorithms();

    /**
     * Get algorithm info by type
     * @param type Algorithm type
     * @return Algorithm information or error
     */
    Result<AlgorithmInfo> getAlgorithmInfo(AlgorithmType type);

    /**
     * Get algorithm type by name (case-insensitive)
     * @param name Algorithm name
     * @return Algorithm type or error
     */
    Result<AlgorithmType> getAlgorithmType(const std::string& name);

    // Delete copy constructor and assignment operator
    EncryptionRegistry(const EncryptionRegistry&) = delete;
    EncryptionRegistry& operator=(const EncryptionRegistry&) = delete;

private:
    EncryptionRegistry() = default;

    std::map<AlgorithmType, EngineCreator> creators;
    std::mutex mutex;
};

/**
 * Helper function to register an algorithm
 * Use this in static initialization
 */
template<typename T>
class AlgorithmRegistrar {
public:
    AlgorithmRegistrar(AlgorithmType type) {
        EncryptionRegistry::instance().registerAlgorithm(type, []() {
            return std::make_unique<T>();
        });
    }
};

// Macro to simplify algorithm registration
#define REGISTER_ALGORITHM(ClassName, AlgoType) \
    static AlgorithmRegistrar<ClassName> registrar_##ClassName(AlgoType)

} // namespace Balance
