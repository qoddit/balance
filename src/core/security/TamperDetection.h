#pragma once

#include "balance/Types.h"
#include <filesystem>

namespace Balance {

/**
 * Tamper detection mechanisms
 * Detects file integrity issues and application tampering
 */
class TamperDetection {
public:
    /**
     * Initialize tamper detection subsystem
     */
    static Result<void> initialize();

    /**
     * Verify file integrity using hash
     * @param filePath File to verify
     * @param expectedHash Expected SHA-256 hash
     * @return true if integrity verified
     */
    static Result<bool> verifyFileIntegrity(
        const std::filesystem::path& filePath,
        const std::vector<uint8_t>& expectedHash);

    /**
     * Calculate file hash
     * @param filePath File to hash
     * @return SHA-256 hash or error
     */
    static Result<std::vector<uint8_t>> calculateFileHash(const std::filesystem::path& filePath);

    /**
     * Verify application binary integrity
     * @return true if application binary is unmodified
     */
    static Result<bool> verifyApplicationIntegrity();

    /**
     * Check if debugger is attached
     * @return true if debugger detected
     */
    static bool isDebuggerAttached();

    /**
     * Log security event
     * @param event Event description
     * @param severity Severity level (0-10)
     */
    static void logSecurityEvent(const std::string& event, int severity);
};

} // namespace Balance
