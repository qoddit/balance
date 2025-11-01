#include "Autocomplete.h"
#include "CommandExecutor.h"
#include <algorithm>

namespace Balance {

std::vector<std::string> Autocomplete::complete(const std::string& partial) {
    std::vector<std::string> matches;

    auto commands = CommandExecutor::instance().listCommands();

    for (const auto& cmd : commands) {
        if (cmd.starts_with(partial)) {
            matches.push_back(cmd);
        }
    }

    return matches;
}

} // namespace Balance
