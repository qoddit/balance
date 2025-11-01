#include "SupabaseProvider.h"

namespace Balance {

Result<void> SupabaseProvider::sendMessage(const Message& message) {
    // TODO: Phase 2 - Implement Supabase message sending
    // Will include E2E encryption and remote sync
    return Error("Supabase provider not yet implemented", -1);
}

Result<std::vector<Message>> SupabaseProvider::getMessages(size_t limit, size_t offset) {
    // TODO: Phase 2 - Implement Supabase message retrieval
    return Error("Supabase provider not yet implemented", -1);
}

Result<void> SupabaseProvider::connect(const std::string& url, const std::string& apiKey) {
    // TODO: Phase 2 - Implement Supabase connection
    // Will use Supabase REST API or real-time websocket
    return Error("Supabase connection not yet implemented", -1);
}

Result<void> SupabaseProvider::subscribe() {
    // TODO: Phase 2 - Subscribe to real-time updates
    return Error("Supabase subscription not yet implemented", -1);
}

void SupabaseProvider::disconnect() {
    // TODO: Phase 2 - Clean disconnect
    connected = false;
}

} // namespace Balance
