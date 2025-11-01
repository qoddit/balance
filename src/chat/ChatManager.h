#pragma once

#include "balance/Types.h"
#include "../core/storage/DatabaseManager.h"
#include <memory>

namespace Balance {

class ChatProvider {
public:
    virtual ~ChatProvider() = default;
    virtual Result<void> sendMessage(const Message& message) = 0;
    virtual Result<std::vector<Message>> getMessages(size_t limit, size_t offset) = 0;
};

class ChatManager {
public:
    static ChatManager& instance();

    Result<void> initialize(const std::string& dbPath);
    Result<void> sendMessage(const std::string& text, const std::string& sender = "local");
    Result<std::vector<Message>> getMessages(size_t limit = 100, size_t offset = 0);
    void openChatWindow();
    void closeChatWindow();
    bool isWindowOpen() const { return windowOpen; }

private:
    ChatManager();

    std::unique_ptr<DatabaseManager> db;
    std::unique_ptr<ChatProvider> provider;
    bool windowOpen;
};

} // namespace Balance
