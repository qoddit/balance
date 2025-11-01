#include "LocalChatProvider.h"

namespace Balance {

LocalChatProvider::LocalChatProvider(DatabaseManager* db) : db(db) {
}

Result<void> LocalChatProvider::sendMessage(const Message& message) {
    if (!db) {
        return Error("Database not initialized", -1);
    }

    auto result = db->storeMessage(message);
    if (result.isError()) {
        return Error(result.error().message, result.error().code);
    }

    return Result<void>();
}

Result<std::vector<Message>> LocalChatProvider::getMessages(size_t limit, size_t offset) {
    if (!db) {
        return Error("Database not initialized", -1);
    }

    return db->getMessages(limit, offset);
}

} // namespace Balance
