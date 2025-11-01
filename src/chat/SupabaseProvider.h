#pragma once

#include "ChatManager.h"
#include "../utils/HttpClient.h"
#include <memory>
#include <string>

namespace Balance {

/**
 * Supabase provider for remote chat with E2E encryption
 * Implements real-time chat synchronization via Supabase REST API
 */
class SupabaseProvider : public ChatProvider {
public:
    SupabaseProvider();
    ~SupabaseProvider() override = default;

    Result<void> sendMessage(const Message& message) override;
    Result<std::vector<Message>> getMessages(size_t limit, size_t offset) override;

    /**
     * Connect to Supabase instance
     * @param url Supabase project URL
     * @param apiKey Supabase anon/service key
     * @return Success or error
     */
    Result<void> connect(const std::string& url, const std::string& apiKey);

    /**
     * Initialize database schema (create tables if needed)
     * @return Success or error
     */
    Result<void> initializeSchema();

    /**
     * Subscribe to real-time updates (future enhancement)
     * @return Success or error
     */
    Result<void> subscribe();

    /**
     * Disconnect from Supabase
     */
    void disconnect();

    /**
     * Check connection status
     * @return true if connected
     */
    bool isConnected() const { return connected; }

    /**
     * Set encryption key for E2E encryption
     * @param key Encryption key for messages
     */
    void setEncryptionKey(const std::vector<uint8_t>& key);

private:
    std::unique_ptr<HttpClient> httpClient;
    std::string supabaseUrl;
    std::string apiKey;
    std::vector<uint8_t> encryptionKey;
    bool connected;

    /**
     * Encrypt message text with E2E encryption
     */
    Result<std::string> encryptMessageText(const std::string& plaintext);

    /**
     * Decrypt message text
     */
    Result<std::string> decryptMessageText(const std::string& ciphertext);

    /**
     * Build authorization headers
     */
    std::map<std::string, std::string> getHeaders();

    /**
     * Parse JSON response to extract messages
     */
    Result<std::vector<Message>> parseMessagesResponse(const std::string& json);

    /**
     * Build JSON for message insert
     */
    std::string buildMessageJson(const Message& message);
};

} // namespace Balance
