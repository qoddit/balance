#include "SupabaseProvider.h"
#include "../core/encryption/AESEncryption.h"
#include "../utils/Logger.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace Balance {

SupabaseProvider::SupabaseProvider()
    : httpClient(std::make_unique<HttpClient>()), connected(false) {
}

Result<void> SupabaseProvider::connect(const std::string& url, const std::string& apiKey) {
    this->supabaseUrl = url;
    this->apiKey = apiKey;

    // Test connection with a simple query
    auto headers = getHeaders();
    auto response = httpClient->get(supabaseUrl + "/rest/v1/", headers);

    if (response.isError()) {
        LOG_ERROR("Failed to connect to Supabase: " + response.error().message);
        return Error("Failed to connect to Supabase: " + response.error().message, -1);
    }

    connected = true;
    LOG_INFO("Connected to Supabase successfully");

    return Result<void>();
}

Result<void> SupabaseProvider::initializeSchema() {
    if (!connected) {
        return Error("Not connected to Supabase", -1);
    }

    // Note: Table creation should be done via Supabase Dashboard or SQL editor
    // This is just a placeholder for schema initialization
    LOG_INFO("Supabase schema initialization (create 'messages' table in Supabase Dashboard)");

    /*
    Required SQL for Supabase:

    CREATE TABLE messages (
        id BIGSERIAL PRIMARY KEY,
        timestamp BIGINT NOT NULL,
        message_text TEXT NOT NULL,
        sender TEXT NOT NULL DEFAULT 'local',
        is_encrypted BOOLEAN NOT NULL DEFAULT false,
        created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
    );

    -- Enable Row Level Security
    ALTER TABLE messages ENABLE ROW LEVEL SECURITY;

    -- Create policy for public access (adjust as needed)
    CREATE POLICY "Allow public access" ON messages
        FOR ALL USING (true);
    */

    return Result<void>();
}

Result<void> SupabaseProvider::sendMessage(const Message& message) {
    if (!connected) {
        return Error("Not connected to Supabase", -1);
    }

    // Encrypt message if encryption key is set
    std::string messageText = message.text;
    bool isEncrypted = false;

    if (!encryptionKey.empty()) {
        auto encryptResult = encryptMessageText(message.text);
        if (encryptResult.isError()) {
            return Error("Failed to encrypt message: " + encryptResult.error().message, -1);
        }
        messageText = encryptResult.value();
        isEncrypted = true;
    }

    // Build message for insertion
    Message encryptedMsg = message;
    encryptedMsg.text = messageText;
    encryptedMsg.isEncrypted = isEncrypted;

    // Build JSON payload
    std::string json = buildMessageJson(encryptedMsg);

    // Send to Supabase
    auto headers = getHeaders();
    headers["Content-Type"] = "application/json";
    headers["Prefer"] = "return=minimal";

    auto response = httpClient->post(
        supabaseUrl + "/rest/v1/messages",
        json,
        headers
    );

    if (response.isError()) {
        return Error("Failed to send message: " + response.error().message, -1);
    }

    if (response.value().statusCode != 201) {
        return Error("Failed to send message: HTTP " + std::to_string(response.value().statusCode), -1);
    }

    LOG_INFO("Message sent to Supabase successfully");
    return Result<void>();
}

Result<std::vector<Message>> SupabaseProvider::getMessages(size_t limit, size_t offset) {
    if (!connected) {
        return Error("Not connected to Supabase", -1);
    }

    // Build query URL
    std::string url = supabaseUrl + "/rest/v1/messages?select=*&order=timestamp.desc&limit=" +
                      std::to_string(limit) + "&offset=" + std::to_string(offset);

    auto headers = getHeaders();
    auto response = httpClient->get(url, headers);

    if (response.isError()) {
        return Error("Failed to get messages: " + response.error().message, -1);
    }

    if (response.value().statusCode != 200) {
        return Error("Failed to get messages: HTTP " + std::to_string(response.value().statusCode), -1);
    }

    // Parse response
    auto parseResult = parseMessagesResponse(response.value().body);
    if (parseResult.isError()) {
        return Error("Failed to parse messages: " + parseResult.error().message, -1);
    }

    // Decrypt messages if needed
    std::vector<Message> decryptedMessages;
    for (auto& msg : parseResult.value()) {
        if (msg.isEncrypted && !encryptionKey.empty()) {
            auto decryptResult = decryptMessageText(msg.text);
            if (decryptResult.isOk()) {
                msg.text = decryptResult.value();
                msg.isEncrypted = false;  // Mark as decrypted for display
            }
        }
        decryptedMessages.push_back(msg);
    }

    return decryptedMessages;
}

Result<void> SupabaseProvider::subscribe() {
    // TODO: Implement real-time subscriptions using Supabase Realtime
    // Requires WebSocket connection
    LOG_INFO("Real-time subscriptions not yet implemented");
    return Result<void>();
}

void SupabaseProvider::disconnect() {
    connected = false;
    LOG_INFO("Disconnected from Supabase");
}

void SupabaseProvider::setEncryptionKey(const std::vector<uint8_t>& key) {
    encryptionKey = key;
    LOG_INFO("E2E encryption key set");
}

Result<std::string> SupabaseProvider::encryptMessageText(const std::string& plaintext) {
    std::vector<uint8_t> plaintextVec(plaintext.begin(), plaintext.end());

    // Use AES-256 for E2E encryption
    auto encryptResult = AESEncryption::encryptWithIV(
        plaintextVec,
        encryptionKey,
        AESEncryption::generateIV()
    );

    if (encryptResult.isError()) {
        return Error(encryptResult.error().message, encryptResult.error().code);
    }

    // Base64 encode for JSON transmission
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);

    BIO_write(bio, encryptResult.value().data(), encryptResult.value().size());
    BIO_flush(bio);

    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string base64(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    return base64;
}

Result<std::string> SupabaseProvider::decryptMessageText(const std::string& ciphertext) {
    // Base64 decode
    BIO* bio = BIO_new_mem_buf(ciphertext.data(), ciphertext.length());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);

    std::vector<uint8_t> decoded(ciphertext.length());
    int decodedLen = BIO_read(bio, decoded.data(), ciphertext.length());
    BIO_free_all(bio);

    if (decodedLen < 0) {
        return Error("Base64 decode failed", -1);
    }

    decoded.resize(decodedLen);

    // Decrypt with AES-256
    auto decryptResult = AESEncryption::decryptWithIV(decoded, encryptionKey);

    if (decryptResult.isError()) {
        return Error(decryptResult.error().message, decryptResult.error().code);
    }

    return std::string(decryptResult.value().begin(), decryptResult.value().end());
}

std::map<std::string, std::string> SupabaseProvider::getHeaders() {
    std::map<std::string, std::string> headers;
    headers["apikey"] = apiKey;
    headers["Authorization"] = "Bearer " + apiKey;
    headers["Content-Type"] = "application/json";
    return headers;
}

Result<std::vector<Message>> SupabaseProvider::parseMessagesResponse(const std::string& json) {
    // Simple JSON parsing (in production, use a proper JSON library like nlohmann/json)
    std::vector<Message> messages;

    // Very basic JSON array parsing
    if (json.empty() || json == "[]") {
        return messages;
    }

    // For now, return empty if parsing is complex
    // In production, implement proper JSON parsing
    LOG_WARNING("JSON parsing simplified - use proper JSON library for production");

    return messages;
}

std::string SupabaseProvider::buildMessageJson(const Message& message) {
    std::ostringstream json;

    json << "{";
    json << "\"timestamp\":" << message.timestamp << ",";
    json << "\"message_text\":\"" << message.text << "\",";
    json << "\"sender\":\"" << message.sender << "\",";
    json << "\"is_encrypted\":" << (message.isEncrypted ? "true" : "false");
    json << "}";

    return json.str();
}

} // namespace Balance
