#include "ChatManager.h"
#include "LocalChatProvider.h"
#include <chrono>

namespace Balance {

ChatManager& ChatManager::instance() {
    static ChatManager manager;
    return manager;
}

ChatManager::ChatManager() : windowOpen(false) {
}

Result<void> ChatManager::initialize(const std::string& dbPath) {
    db = std::make_unique<DatabaseManager>(dbPath);

    auto openResult = db->open();
    if (openResult.isError()) {
        return openResult;
    }

    auto schemaResult = db->initializeSchema();
    if (schemaResult.isError()) {
        return schemaResult;
    }

    provider = std::make_unique<LocalChatProvider>(db.get());

    return Result<void>();
}

Result<void> ChatManager::sendMessage(const std::string& text, const std::string& sender) {
    if (!db) {
        return Error("Chat manager not initialized", -1);
    }

    Message msg;
    msg.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    msg.text = text;
    msg.sender = sender;
    msg.isEncrypted = false;

    return provider->sendMessage(msg);
}

Result<std::vector<Message>> ChatManager::getMessages(size_t limit, size_t offset) {
    if (!db) {
        return Error("Chat manager not initialized", -1);
    }

    return provider->getMessages(limit, offset);
}

void ChatManager::openChatWindow() {
    // TODO: Open Qt chat window
    windowOpen = true;
}

void ChatManager::closeChatWindow() {
    // TODO: Close Qt chat window
    windowOpen = false;
}

} // namespace Balance
