#pragma once

#include "CommandParser.h"
#include "balance/Types.h"
#include <functional>
#include <map>

namespace Balance {

/**
 * Command handler function type
 */
using CommandHandler = std::function<Result<std::string>(const Command&)>;

/**
 * Command executor registry
 */
class CommandExecutor {
public:
    static CommandExecutor& instance();

    /**
     * Register a command handler
     */
    void registerCommand(const std::string& name, CommandHandler handler, const std::string& description);

    /**
     * Execute a command
     */
    Result<std::string> execute(const Command& cmd);

    /**
     * Get list of all registered commands
     */
    std::vector<std::string> listCommands() const;

    /**
     * Get command description
     */
    std::string getDescription(const std::string& name) const;

private:
    CommandExecutor();

    std::map<std::string, CommandHandler> handlers;
    std::map<std::string, std::string> descriptions;
};

} // namespace Balance
