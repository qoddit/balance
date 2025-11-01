#pragma once

#include <string>

namespace Balance {

/**
 * Input validation utilities
 */
class Validators {
public:
    /**
     * Validate password strength
     * @param password Password to validate
     * @return true if password meets requirements
     */
    static bool validatePassword(const std::string& password);

    /**
     * Validate file path
     * @param path File path to validate
     * @return true if path is valid
     */
    static bool validateFilePath(const std::string& path);

    /**
     * Validate identifier (alphanumeric with dashes/underscores)
     * @param identifier Identifier to validate
     * @return true if valid
     */
    static bool validateIdentifier(const std::string& identifier);

    /**
     * Validate algorithm name
     * @param name Algorithm name
     * @return true if valid algorithm name
     */
    static bool validateAlgorithmName(const std::string& name);

    /**
     * Get password strength (0-100)
     * @param password Password to check
     * @return Strength score
     */
    static int getPasswordStrength(const std::string& password);
};

} // namespace Balance
