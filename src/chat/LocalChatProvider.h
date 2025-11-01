#pragma once

#include "ChatManager.h"
#include "../core/storage/DatabaseManager.h"

namespace Balance {

class LocalChatProvider : public ChatProvider {
public:
    explicit LocalChatProvider(DatabaseManager* db);
    ~LocalChatProvider() override = default;

    Result<void> sendMessage(const Message& message) override;
    Result<std::vector<Message>> getMessages(size_t limit, size_t offset) override;

private:
    DatabaseManager* db;
};

} // namespace Balance
