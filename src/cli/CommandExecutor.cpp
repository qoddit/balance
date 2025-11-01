#include "CommandExecutor.h"
#include "../utils/Logger.h"

namespace Balance {

CommandExecutor& CommandExecutor::instance() {
    static CommandExecutor executor;
    return executor;
}

CommandExecutor::CommandExecutor() {
    // Commands are registered by individual command implementations
}

void CommandExecutor::registerCommand(const std::string& name, CommandHandler handler, const std::string& description) {
    handlers[name] = handler;
    descriptions[name] = description;
}

Result<std::string> CommandExecutor::execute(const Command& cmd) {
    auto it = handlers.find(cmd.name);
    if (it == handlers.end()) {
        return Error("Unknown command: " + cmd.name, -1);
    }

    LOG_INFO("Executing command: " + cmd.name);

    try {
        return it->second(cmd);
    } catch (const std::exception& e) {
        return Error("Command execution failed: " + std::string(e.what()), -1);
    }
}

std::vector<std::string> CommandExecutor::listCommands() const {
    std::vector<std::string> commands;
    commands.reserve(handlers.size());

    for (const auto& [name, _] : handlers) {
        commands.push_back(name);
    }

    return commands;
}

std::string CommandExecutor::getDescription(const std::string& name) const {
    auto it = descriptions.find(name);
    if (it != descriptions.end()) {
        return it->second;
    }
    return "";
}

} // namespace Balance
