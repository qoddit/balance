#include <gtest/gtest.h>
#include "cli/CommandParser.h"

using namespace Balance;

TEST(CommandParserTest, BasicCommand) {
    auto result = CommandParser::parse("help");

    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result.value().name, "help");
    EXPECT_TRUE(result.value().args.empty());
    EXPECT_TRUE(result.value().flags.empty());
}

TEST(CommandParserTest, CommandWithArgs) {
    auto result = CommandParser::parse("encrypt file.txt output.balance");

    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result.value().name, "encrypt");
    ASSERT_EQ(result.value().args.size(), 2);
    EXPECT_EQ(result.value().args[0], "file.txt");
    EXPECT_EQ(result.value().args[1], "output.balance");
}

TEST(CommandParserTest, CommandWithFlags) {
    auto result = CommandParser::parse("encrypt file.txt --algorithm aes --password test");

    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result.value().name, "encrypt");
    ASSERT_EQ(result.value().args.size(), 1);
    EXPECT_EQ(result.value().flags["algorithm"], "aes");
    EXPECT_EQ(result.value().flags["password"], "test");
}

TEST(CommandParserTest, QuotedArguments) {
    auto result = CommandParser::parse("encrypt \"file with spaces.txt\"");

    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(result.value().args.size(), 1);
    EXPECT_EQ(result.value().args[0], "file with spaces.txt");
}
