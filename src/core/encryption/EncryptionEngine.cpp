#include "EncryptionEngine.h"
#include <algorithm>
#include <cctype>

namespace Balance {

EncryptionRegistry& EncryptionRegistry::instance() {
    static EncryptionRegistry registry;
    return registry;
}

void EncryptionRegistry::registerAlgorithm(AlgorithmType type, EngineCreator creator) {
    std::lock_guard<std::mutex> lock(mutex);
    creators[type] = std::move(creator);
}

Result<std::unique_ptr<EncryptionEngine>> EncryptionRegistry::createEngine(AlgorithmType type) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = creators.find(type);
    if (it == creators.end()) {
        return Error("Algorithm not registered");
    }

    return it->second();
}

std::vector<AlgorithmInfo> EncryptionRegistry::getSupportedAlgorithms() {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<AlgorithmInfo> algorithms;
    algorithms.reserve(creators.size());

    for (const auto& [type, creator] : creators) {
        auto engine = creator();
        algorithms.push_back(engine->getInfo());
    }

    return algorithms;
}

Result<AlgorithmInfo> EncryptionRegistry::getAlgorithmInfo(AlgorithmType type) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = creators.find(type);
    if (it == creators.end()) {
        return Error("Algorithm not found");
    }

    auto engine = it->second();
    return engine->getInfo();
}

Result<AlgorithmType> EncryptionRegistry::getAlgorithmType(const std::string& name) {
    // Convert name to lowercase for case-insensitive comparison
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Map algorithm names to types
    static const std::map<std::string, AlgorithmType> nameMap = {
        {"balance", AlgorithmType::Balance},
        {"aes", AlgorithmType::AES256},
        {"aes256", AlgorithmType::AES256},
        {"aes-256", AlgorithmType::AES256},
        {"rsa", AlgorithmType::RSA4096},
        {"rsa4096", AlgorithmType::RSA4096},
        {"rsa-4096", AlgorithmType::RSA4096},
        {"serpent", AlgorithmType::Serpent},
        {"blowfish", AlgorithmType::Blowfish}
    };

    auto it = nameMap.find(lowerName);
    if (it == nameMap.end()) {
        return Error("Unknown algorithm: " + name);
    }

    return it->second;
}

} // namespace Balance
