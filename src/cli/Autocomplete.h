#pragma once

#include <string>
#include <vector>

namespace Balance {

class Autocomplete {
public:
    static std::vector<std::string> complete(const std::string& partial);
};

} // namespace Balance
