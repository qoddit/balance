#include "MessageStore.h"

namespace Balance {

MessageStore::MessageStore(DatabaseManager* db) : db(db) {
}

Result<uint64_t> MessageStore::store(const Message& message) {
    return db->storeMessage(message);
}

Result<std::vector<Message>> MessageStore::retrieve(size_t limit, size_t offset) {
    return db->getMessages(limit, offset);
}

Result<void> MessageStore::remove(uint64_t messageId) {
    return db->deleteMessage(messageId);
}

Result<void> MessageStore::clearAll() {
    return db->clearMessages();
}

} // namespace Balance
