#pragma once

#include <string>
#include <vector>

namespace Balance {

class CommandHistory {
public:
    static CommandHistory& instance();

    void add(const std::string& command);
    std::vector<std::string> getHistory() const;
    void clear();
    std::string getPrevious();
    std::string getNext();

private:
    CommandHistory() : currentIndex(0) {}

    std::vector<std::string> history;
    size_t currentIndex;
};

} // namespace Balance
