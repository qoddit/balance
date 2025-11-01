#include <gtest/gtest.h>
#include "core/encryption/BalanceFormat.h"

using namespace Balance;

TEST(BalanceFormatTest, HeaderCreation) {
    // Test header creation and parsing
    std::vector<uint8_t> salt(16, 0xAB);
    auto header = BalanceFormat::createHeader(1024, salt);

    EXPECT_EQ(header.size(), BalanceHeader::HEADER_SIZE);

    auto parseResult = BalanceFormat::parseHeader(header);
    ASSERT_TRUE(parseResult.isOk());
    EXPECT_EQ(parseResult.value().originalSize, 1024);
}

TEST(BalanceFormatTest, KeyDerivation) {
    std::string password = "TestPassword123!";
    std::vector<uint8_t> salt(16, 0x42);

    auto keyResult = BalanceFormat::deriveKeyFromPassword(password, salt);

    ASSERT_TRUE(keyResult.isOk());
    EXPECT_EQ(keyResult.value().size(), 32);  // 256 bits
}

// Add more tests for file encryption/decryption when test files are available
