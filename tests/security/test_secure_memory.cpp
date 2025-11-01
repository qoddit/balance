#include <gtest/gtest.h>
#include "core/security/SecureMemory.h"

using namespace Balance;

TEST(SecureMemoryTest, Initialization) {
    EXPECT_TRUE(SecureMemory::initialize());
}

TEST(SecureMemoryTest, SecureBufferBasic) {
    SecureBuffer buffer(32);

    EXPECT_EQ(buffer.size(), 32);
    EXPECT_FALSE(buffer.empty());

    buffer.clear();
    EXPECT_TRUE(buffer.empty());
}

TEST(SecureMemoryTest, SecureBufferWithData) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    SecureBuffer buffer(data.data(), data.size());

    EXPECT_EQ(buffer.size(), 5);
    EXPECT_EQ(buffer[0], 1);
    EXPECT_EQ(buffer[4], 5);
}

TEST(SecureMemoryTest, SecureStringBasic) {
    SecureString str("TestPassword");

    EXPECT_EQ(str.length(), 12);
    EXPECT_EQ(str.toString(), "TestPassword");
}

TEST(SecureMemoryTest, SecureStringComparison) {
    SecureString str1("password");
    SecureString str2("password");
    SecureString str3("different");

    EXPECT_TRUE(str1 == str2);
    EXPECT_FALSE(str1 == str3);
}
