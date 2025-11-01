#include "FIPSModule.h"
#include <openssl/crypto.h>
#include <openssl/evp.h>

namespace Balance {

bool FIPSModule::fipsEnabled = false;

Result<void> FIPSModule::initialize(bool enableFIPS) {
    if (enableFIPS) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
        // OpenSSL 3.0+ FIPS mode
        // Note: Requires OpenSSL compiled with FIPS module
        if (EVP_default_properties_is_fips_enabled(nullptr)) {
            fipsEnabled = true;
            return Result<void>();
        } else {
            // Try to enable FIPS mode
            if (EVP_default_properties_enable_fips(nullptr, 1)) {
                fipsEnabled = true;
                return Result<void>();
            } else {
                return Error("Failed to enable FIPS mode (OpenSSL may not be compiled with FIPS support)", -1);
            }
        }
#else
        // OpenSSL 1.x FIPS mode
        if (FIPS_mode_set(1) == 1) {
            fipsEnabled = true;
            return Result<void>();
        } else {
            return Error("Failed to enable FIPS mode (OpenSSL may not be compiled with FIPS support)", -1);
        }
#endif
    } else {
        fipsEnabled = false;
    }

    return Result<void>();
}

Result<bool> FIPSModule::performSelfTests() {
    // OpenSSL performs self-tests automatically when entering FIPS mode
    // Additional self-tests can be added here

    if (fipsEnabled) {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
        // OpenSSL 3.0+ self-test status
        if (EVP_default_properties_is_fips_enabled(nullptr)) {
            return true;
        }
        return false;
#else
        // OpenSSL 1.x self-test status
        if (FIPS_mode() == 1) {
            return true;
        }
        return false;
#endif
    }

    // Not in FIPS mode, tests not applicable
    return true;
}

bool FIPSModule::isFIPSEnabled() {
    return fipsEnabled;
}

std::string FIPSModule::getVersion() {
    return "Balance FIPS Module v1.0 (OpenSSL " + std::string(OPENSSL_VERSION_TEXT) + ")";
}

} // namespace Balance
