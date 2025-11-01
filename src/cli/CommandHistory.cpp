#include "CommandHistory.h"

namespace Balance {

CommandHistory& CommandHistory::instance() {
    static CommandHistory hist;
    return hist;
}

void CommandHistory::add(const std::string& command) {
    if (!command.empty()) {
        history.push_back(command);
        currentIndex = history.size();
    }
}

std::vector<std::string> CommandHistory::getHistory() const {
    return history;
}

void CommandHistory::clear() {
    history.clear();
    currentIndex = 0;
}

std::string CommandHistory::getPrevious() {
    if (history.empty()) return "";
    if (currentIndex > 0) currentIndex--;
    return history[currentIndex];
}

std::string CommandHistory::getNext() {
    if (history.empty()) return "";
    if (currentIndex < history.size() - 1) currentIndex++;
    return history[currentIndex];
}

} // namespace Balance
