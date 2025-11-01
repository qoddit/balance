#include "Config.h"
#include <fstream>
#include <sstream>

namespace Balance {

Config& Config::instance() {
    static Config config;
    return config;
}

Config::Config() {
    // Set defaults
    values["default_algorithm"] = "balance";
    values["keystore_path"] = "./keys";
    values["database_path"] = "./balance.db";
    values["max_memory_usage"] = "209715200";  // 200MB
    values["streaming_threshold"] = "104857600";  // 100MB
    values["enable_fips"] = "false";
    values["enable_audit_log"] = "true";
}

Result<void> Config::load(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex);

    std::ifstream file(filePath);
    if (!file) {
        return Error("Failed to open config file: " + filePath, -1);
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse key=value pairs
        auto pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            values[key] = value;
        }
    }

    file.close();
    return Result<void>();
}

Result<void> Config::save(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex);

    std::ofstream file(filePath);
    if (!file) {
        return Error("Failed to open config file for writing: " + filePath, -1);
    }

    file << "# Balance Configuration File\n\n";

    for (const auto& [key, value] : values) {
        file << key << " = " << value << "\n";
    }

    file.close();
    return Result<void>();
}

std::string Config::get(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = values.find(key);
    if (it != values.end()) {
        return it->second;
    }

    return defaultValue;
}

void Config::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex);
    values[key] = value;
}

int Config::getInt(const std::string& key, int defaultValue) const {
    std::string value = get(key);
    if (value.empty()) {
        return defaultValue;
    }

    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    std::string value = get(key);
    if (value.empty()) {
        return defaultValue;
    }

    return value == "true" || value == "1" || value == "yes";
}

Configuration Config::getConfiguration() const {
    std::lock_guard<std::mutex> lock(mutex);

    Configuration config;
    config.defaultAlgorithm = get("default_algorithm", "balance");
    config.keyStorePath = get("keystore_path", "./keys");
    config.databasePath = get("database_path", "./balance.db");
    config.maxMemoryUsage = getInt("max_memory_usage", 200 * 1024 * 1024);
    config.streamingThreshold = getInt("streaming_threshold", 100 * 1024 * 1024);
    config.enableFIPS = getBool("enable_fips", false);
    config.enableAuditLog = getBool("enable_audit_log", true);

    return config;
}

void Config::setConfiguration(const Configuration& config) {
    std::lock_guard<std::mutex> lock(mutex);

    values["default_algorithm"] = config.defaultAlgorithm;
    values["keystore_path"] = config.keyStorePath;
    values["database_path"] = config.databasePath;
    values["max_memory_usage"] = std::to_string(config.maxMemoryUsage);
    values["streaming_threshold"] = std::to_string(config.streamingThreshold);
    values["enable_fips"] = config.enableFIPS ? "true" : "false";
    values["enable_audit_log"] = config.enableAuditLog ? "true" : "false";
}

} // namespace Balance
