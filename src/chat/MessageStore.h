#pragma once

#include "balance/Types.h"
#include "../core/storage/DatabaseManager.h"
#include <memory>

namespace Balance {

/**
 * Message storage abstraction
 */
class MessageStore {
public:
    explicit MessageStore(DatabaseManager* db);

    Result<uint64_t> store(const Message& message);
    Result<std::vector<Message>> retrieve(size_t limit, size_t offset);
    Result<void> remove(uint64_t messageId);
    Result<void> clearAll();

private:
    DatabaseManager* db;
};

} // namespace Balance
