#pragma once

#include "balance/Types.h"
#include <string>
#include <map>
#include <mutex>

namespace Balance {

/**
 * Configuration management
 * Handles loading/saving application configuration
 */
class Config {
public:
    static Config& instance();

    /**
     * Load configuration from file
     */
    Result<void> load(const std::string& filePath);

    /**
     * Save configuration to file
     */
    Result<void> save(const std::string& filePath);

    /**
     * Get configuration value
     */
    std::string get(const std::string& key, const std::string& defaultValue = "") const;

    /**
     * Set configuration value
     */
    void set(const std::string& key, const std::string& value);

    /**
     * Get integer value
     */
    int getInt(const std::string& key, int defaultValue = 0) const;

    /**
     * Get boolean value
     */
    bool getBool(const std::string& key, bool defaultValue = false) const;

    /**
     * Get all configuration as Configuration struct
     */
    Configuration getConfiguration() const;

    /**
     * Set configuration from Configuration struct
     */
    void setConfiguration(const Configuration& config);

private:
    Config();

    std::map<std::string, std::string> values;
    mutable std::mutex mutex;
};

} // namespace Balance
