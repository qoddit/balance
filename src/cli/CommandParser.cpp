#include "CommandParser.h"
#include <sstream>

namespace Balance {

Result<Command> CommandParser::parse(const std::string& input) {
    if (input.empty()) {
        return Error("Empty command", -1);
    }

    auto tokens = tokenize(input);
    if (tokens.empty()) {
        return Error("No command found", -1);
    }

    Command cmd;
    cmd.name = tokens[0];

    // Parse arguments and flags
    for (size_t i = 1; i < tokens.size(); ++i) {
        const auto& token = tokens[i];

        if (token.starts_with("--")) {
            // Long flag
            std::string flag = token.substr(2);
            auto pos = flag.find('=');

            if (pos != std::string::npos) {
                std::string key = flag.substr(0, pos);
                std::string value = flag.substr(pos + 1);
                cmd.flags[key] = value;
            } else if (i + 1 < tokens.size() && !tokens[i + 1].starts_with("-")) {
                cmd.flags[flag] = tokens[++i];
            } else {
                cmd.flags[flag] = "true";
            }
        } else if (token.starts_with("-")) {
            // Short flag
            std::string flag = token.substr(1);
            if (i + 1 < tokens.size() && !tokens[i + 1].starts_with("-")) {
                cmd.flags[flag] = tokens[++i];
            } else {
                cmd.flags[flag] = "true";
            }
        } else {
            // Argument
            cmd.args.push_back(token);
        }
    }

    return cmd;
}

std::vector<std::string> CommandParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    bool escapeNext = false;

    for (char c : input) {
        if (escapeNext) {
            current += c;
            escapeNext = false;
            continue;
        }

        if (c == '\\') {
            escapeNext = true;
            continue;
        }

        if (c == '"') {
            inQuotes = !inQuotes;
            continue;
        }

        if (std::isspace(c) && !inQuotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

} // namespace Balance
