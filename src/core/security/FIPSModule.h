#pragma once

#include "balance/Types.h"

namespace Balance {

/**
 * FIPS 140-2 compliance module
 * Handles FIPS mode initialization and self-tests
 */
class FIPSModule {
public:
    /**
     * Initialize FIPS mode
     * @param enableFIPS Enable FIPS 140-2 mode
     * @return Success or error
     */
    static Result<void> initialize(bool enableFIPS = false);

    /**
     * Perform FIPS self-tests
     * @return true if all self-tests pass
     */
    static Result<bool> performSelfTests();

    /**
     * Check if FIPS mode is enabled
     * @return true if FIPS mode is active
     */
    static bool isFIPSEnabled();

    /**
     * Get FIPS module version
     * @return Version string
     */
    static std::string getVersion();

private:
    static bool fipsEnabled;
};

} // namespace Balance
