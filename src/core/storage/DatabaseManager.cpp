#include "DatabaseManager.h"
#include <cstring>
#include <chrono>

namespace Balance {

DatabaseManager::DatabaseManager(const std::string& dbPath)
    : dbPath(dbPath), db(nullptr), isOpen(false) {
}

DatabaseManager::~DatabaseManager() {
    close();
}

Result<void> DatabaseManager::open() {
    std::lock_guard<std::mutex> lock(mutex);

    if (isOpen) {
        return Result<void>();
    }

    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::string error = getErrorMessage();
        sqlite3_close(db);
        db = nullptr;
        return Error("Failed to open database: " + error, rc);
    }

    isOpen = true;
    return Result<void>();
}

void DatabaseManager::close() {
    std::lock_guard<std::mutex> lock(mutex);

    if (db) {
        sqlite3_close(db);
        db = nullptr;
        isOpen = false;
    }
}

Result<void> DatabaseManager::initializeSchema() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen) {
        return Error("Database not open", -1);
    }

    // Create messages table
    const char* createMessages = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            message_text TEXT NOT NULL,
            sender TEXT NOT NULL DEFAULT 'local',
            is_encrypted INTEGER NOT NULL DEFAULT 0
        );
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, createMessages, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::string error = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        return Error("Failed to create schema: " + error, rc);
    }

    // Create index on timestamp
    const char* createIndex = "CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON messages(timestamp);";

    rc = sqlite3_exec(db, createIndex, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::string error = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        return Error("Failed to create index: " + error, rc);
    }

    return Result<void>();
}

Result<void> DatabaseManager::execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen) {
        return Error("Database not open", -1);
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::string error = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        return Error("SQL execution failed: " + error, rc);
    }

    return Result<void>();
}

Result<void> DatabaseManager::executeWithParams(const std::string& sql, const std::vector<std::string>& params) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen) {
        return Error("Database not open", -1);
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return Error("Failed to prepare statement: " + getErrorMessage(), rc);
    }

    // Bind parameters
    for (size_t i = 0; i < params.size(); ++i) {
        rc = sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            sqlite3_finalize(stmt);
            return Error("Failed to bind parameter: " + getErrorMessage(), rc);
        }
    }

    // Execute
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        return Error("Failed to execute statement: " + getErrorMessage(), rc);
    }

    return Result<void>();
}

Result<DatabaseManager::QueryResult> DatabaseManager::query(const std::string& sql) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen) {
        return Error("Database not open", -1);
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return Error("Failed to prepare query: " + getErrorMessage(), rc);
    }

    QueryResult result;

    // Get column names
    int columnCount = sqlite3_column_count(stmt);
    for (int i = 0; i < columnCount; ++i) {
        result.columnNames.push_back(sqlite3_column_name(stmt, i));
    }

    // Fetch rows
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::vector<std::string> row;
        for (int i = 0; i < columnCount; ++i) {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
            row.push_back(text ? text : "");
        }
        result.rows.push_back(std::move(row));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Error("Query execution failed: " + getErrorMessage(), rc);
    }

    return result;
}

Result<void> DatabaseManager::beginTransaction() {
    return execute("BEGIN TRANSACTION");
}

Result<void> DatabaseManager::commitTransaction() {
    return execute("COMMIT");
}

Result<void> DatabaseManager::rollbackTransaction() {
    return execute("ROLLBACK");
}

int64_t DatabaseManager::getLastInsertId() const {
    if (!db) {
        return -1;
    }
    return sqlite3_last_insert_rowid(db);
}

bool DatabaseManager::checkIntegrity() {
    auto result = query("PRAGMA integrity_check;");
    if (result.isError()) {
        return false;
    }

    return !result.value().rows.empty() &&
           result.value().rows[0][0] == "ok";
}

Result<uint64_t> DatabaseManager::storeMessage(const Message& message) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen) {
        return Error("Database not open", -1);
    }

    const char* sql = "INSERT INTO messages (timestamp, message_text, sender, is_encrypted) VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return Error("Failed to prepare statement: " + getErrorMessage(), rc);
    }

    // Bind parameters
    sqlite3_bind_int64(stmt, 1, message.timestamp);
    sqlite3_bind_text(stmt, 2, message.text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, message.sender.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, message.isEncrypted ? 1 : 0);

    // Execute
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Error("Failed to insert message: " + getErrorMessage(), rc);
    }

    return static_cast<uint64_t>(getLastInsertId());
}

Result<std::vector<Message>> DatabaseManager::getMessages(size_t limit, size_t offset) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen) {
        return Error("Database not open", -1);
    }

    std::string sql = "SELECT id, timestamp, message_text, sender, is_encrypted FROM messages "
                      "ORDER BY timestamp DESC LIMIT " + std::to_string(limit) +
                      " OFFSET " + std::to_string(offset) + ";";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return Error("Failed to prepare query: " + getErrorMessage(), rc);
    }

    std::vector<Message> messages;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Message msg;
        msg.id = sqlite3_column_int64(stmt, 0);
        msg.timestamp = sqlite3_column_int64(stmt, 1);
        msg.text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        msg.sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        msg.isEncrypted = sqlite3_column_int(stmt, 4) != 0;

        messages.push_back(std::move(msg));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return Error("Failed to fetch messages: " + getErrorMessage(), rc);
    }

    return messages;
}

Result<void> DatabaseManager::deleteMessage(uint64_t messageId) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!isOpen) {
        return Error("Database not open", -1);
    }

    std::string sql = "DELETE FROM messages WHERE id = " + std::to_string(messageId) + ";";
    return execute(sql);
}

Result<void> DatabaseManager::clearMessages() {
    return execute("DELETE FROM messages;");
}

std::string DatabaseManager::getErrorMessage() const {
    if (!db) {
        return "Database not initialized";
    }
    return sqlite3_errmsg(db);
}

} // namespace Balance
