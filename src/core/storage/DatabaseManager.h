#pragma once

#include "balance/Types.h"
#include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace Balance {

/**
 * SQLite database manager
 * Handles database operations for chat messages and application data
 */
class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& dbPath);
    ~DatabaseManager();

    /**
     * Open database connection
     * @return Success or error
     */
    Result<void> open();

    /**
     * Close database connection
     */
    void close();

    /**
     * Initialize database schema
     * @return Success or error
     */
    Result<void> initializeSchema();

    /**
     * Execute SQL query
     * @param sql SQL statement
     * @return Success or error
     */
    Result<void> execute(const std::string& sql);

    /**
     * Execute SQL query with parameters
     * @param sql SQL statement with placeholders
     * @param params Parameters to bind
     * @return Success or error
     */
    Result<void> executeWithParams(const std::string& sql, const std::vector<std::string>& params);

    /**
     * Query database and return results
     * @param sql SQL SELECT statement
     * @return Result rows or error
     */
    struct QueryResult {
        std::vector<std::string> columnNames;
        std::vector<std::vector<std::string>> rows;
    };

    Result<QueryResult> query(const std::string& sql);

    /**
     * Begin transaction
     * @return Success or error
     */
    Result<void> beginTransaction();

    /**
     * Commit transaction
     * @return Success or error
     */
    Result<void> commitTransaction();

    /**
     * Rollback transaction
     * @return Success or error
     */
    Result<void> rollbackTransaction();

    /**
     * Get last insert row ID
     * @return Row ID
     */
    int64_t getLastInsertId() const;

    /**
     * Check database integrity
     * @return true if integrity check passes
     */
    bool checkIntegrity();

    // Message-specific operations

    /**
     * Store a message
     * @param message Message to store
     * @return Message ID or error
     */
    Result<uint64_t> storeMessage(const Message& message);

    /**
     * Retrieve messages
     * @param limit Number of messages to retrieve
     * @param offset Offset for pagination
     * @return List of messages or error
     */
    Result<std::vector<Message>> getMessages(size_t limit = 100, size_t offset = 0);

    /**
     * Delete a message
     * @param messageId Message ID
     * @return Success or error
     */
    Result<void> deleteMessage(uint64_t messageId);

    /**
     * Clear all messages
     * @return Success or error
     */
    Result<void> clearMessages();

    // Delete copy constructor and assignment
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

private:
    std::string dbPath;
    sqlite3* db;
    std::mutex mutex;
    bool isOpen;

    /**
     * Get error message from SQLite
     */
    std::string getErrorMessage() const;
};

} // namespace Balance
