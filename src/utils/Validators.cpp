#include "Validators.h"
#include <algorithm>
#include <cctype>

namespace Balance {

bool Validators::validatePassword(const std::string& password) {
    // Minimum 12 characters
    if (password.length() < 12) {
        return false;
    }

    // Check for required character types
    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;
    bool hasSpecial = false;

    for (char c : password) {
        if (std::isupper(c)) hasUpper = true;
        else if (std::islower(c)) hasLower = true;
        else if (std::isdigit(c)) hasDigit = true;
        else hasSpecial = true;
    }

    // Require at least 3 of 4 character types
    int typeCount = (hasUpper ? 1 : 0) + (hasLower ? 1 : 0) + (hasDigit ? 1 : 0) + (hasSpecial ? 1 : 0);

    return typeCount >= 3;
}

bool Validators::validateFilePath(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    // Basic validation - check for invalid characters
    for (char c : path) {
        if (c == '\0' || c == '\n' || c == '\r') {
            return false;
        }
    }

    return true;
}

bool Validators::validateIdentifier(const std::string& identifier) {
    if (identifier.empty()) {
        return false;
    }

    // Check if all characters are alphanumeric, dash, or underscore
    return std::all_of(identifier.begin(), identifier.end(), [](char c) {
        return std::isalnum(c) || c == '-' || c == '_';
    });
}

bool Validators::validateAlgorithmName(const std::string& name) {
    static const std::vector<std::string> validNames = {
        "balance", "aes", "aes256", "aes-256",
        "rsa", "rsa4096", "rsa-4096",
        "serpent", "blowfish"
    };

    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return std::find(validNames.begin(), validNames.end(), lowerName) != validNames.end();
}

int Validators::getPasswordStrength(const std::string& password) {
    int score = 0;

    // Length score (up to 30 points)
    if (password.length() >= 12) score += 10;
    if (password.length() >= 16) score += 10;
    if (password.length() >= 20) score += 10;

    // Character type scores
    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;
    bool hasSpecial = false;

    for (char c : password) {
        if (std::isupper(c)) hasUpper = true;
        else if (std::islower(c)) hasLower = true;
        else if (std::isdigit(c)) hasDigit = true;
        else hasSpecial = true;
    }

    if (hasUpper) score += 15;
    if (hasLower) score += 15;
    if (hasDigit) score += 15;
    if (hasSpecial) score += 25;

    return std::min(score, 100);
}

} // namespace Balance
