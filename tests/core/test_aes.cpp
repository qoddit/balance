#include <gtest/gtest.h>
#include "core/encryption/AESEncryption.h"

using namespace Balance;

TEST(AESEncryptionTest, KeyGeneration) {
    AESEncryption aes;
    auto keyResult = aes.generateKey();

    ASSERT_TRUE(keyResult.isOk());
    EXPECT_EQ(keyResult.value().data.size(), 32);  // 256 bits
}

TEST(AESEncryptionTest, EncryptDecryptRoundtrip) {
    AESEncryption aes;
    auto keyResult = aes.generateKey();
    ASSERT_TRUE(keyResult.isOk());

    std::vector<uint8_t> plaintext = {1, 2, 3, 4, 5};

    auto encryptResult = aes.encrypt(plaintext, keyResult.value());
    ASSERT_TRUE(encryptResult.isOk());

    auto decryptResult = aes.decrypt(encryptResult.value(), keyResult.value());
    ASSERT_TRUE(decryptResult.isOk());

    EXPECT_EQ(decryptResult.value(), plaintext);
}

TEST(AESEncryptionTest, InvalidKey) {
    AESEncryption aes;
    std::vector<uint8_t> plaintext = {1, 2, 3};
    EncryptionKey invalidKey(KeyType::Symmetric, {1, 2, 3}, "invalid");

    auto result = aes.encrypt(plaintext, invalidKey);
    ASSERT_TRUE(result.isError());
}
