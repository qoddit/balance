#pragma once

#include "balance/Types.h"
#include <string>
#include <vector>
#include <map>

namespace Balance {

/**
 * Parsed command structure
 */
struct Command {
    std::string name;
    std::vector<std::string> args;
    std::map<std::string, std::string> flags;
};

/**
 * Command parser for CLI
 */
class CommandParser {
public:
    /**
     * Parse command string
     * @param input Raw command string
     * @return Parsed command or error
     */
    static Result<Command> parse(const std::string& input);

private:
    static std::vector<std::string> tokenize(const std::string& input);
};

} // namespace Balance
