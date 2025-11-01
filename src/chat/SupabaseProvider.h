#pragma once

#include "ChatManager.h"

namespace Balance {

/**
 * Supabase provider stub for Phase 2
 * Will be implemented when Supabase integration is added
 */
class SupabaseProvider : public ChatProvider {
public:
    SupabaseProvider() = default;
    ~SupabaseProvider() override = default;

    Result<void> sendMessage(const Message& message) override;
    Result<std::vector<Message>> getMessages(size_t limit, size_t offset) override;

    // Phase 2: Connection and sync methods
    Result<void> connect(const std::string& url, const std::string& apiKey);
    Result<void> subscribe();
    void disconnect();

private:
    bool connected = false;
};

} // namespace Balance
