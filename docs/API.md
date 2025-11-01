# Balance - API Documentation

## Overview

Balance provides a comprehensive C++ API for encryption operations, key management, and secure data handling.

## Core Namespace

All Balance components are in the `Balance` namespace:

```cpp
using namespace Balance;
```

## Encryption API

### EncryptionEngine

Base class for all encryption algorithms.

```cpp
class EncryptionEngine {
public:
    virtual Result<std::vector<uint8_t>> encrypt(
        const std::vector<uint8_t>& data,
        const EncryptionKey& key) = 0;

    virtual Result<std::vector<uint8_t>> decrypt(
        const std::vector<uint8_t>& data,
        const EncryptionKey& key) = 0;

    virtual AlgorithmInfo getInfo() const = 0;
    virtual Result<EncryptionKey> generateKey() = 0;
};
```

### Usage Example

```cpp
#include "balance/Balance.h"

// Create AES encryption engine
auto engine = EncryptionRegistry::instance().createEngine(AlgorithmType::AES256);
if (engine.isError()) {
    // Handle error
}

// Generate key
auto key = engine.value()->generateKey();

// Encrypt data
std::vector<uint8_t> plaintext = {/* your data */};
auto encrypted = engine.value()->encrypt(plaintext, key.value());

// Decrypt data
auto decrypted = engine.value()->decrypt(encrypted.value(), key.value());
```

### Balance Format API

```cpp
// Encrypt file with Balance format
auto result = BalanceFormat::encryptFile(
    "input.txt",
    "output.balance",
    "password123"
);

// Decrypt file
auto result = BalanceFormat::decryptFile(
    "output.balance",
    "decrypted.txt",
    "password123"
);

// Verify integrity
auto isValid = BalanceFormat::verifyIntegrity("output.balance");
```

## Key Management API

### KeyManager

Singleton for managing encryption keys.

```cpp
// Generate key
auto key = KeyManager::instance().generateKey(
    AlgorithmType::AES256,
    "my-key-id"
);

// Store key
KeyManager::instance().storeKey(key.value(), true);  // persistent

// Retrieve key
auto storedKey = KeyManager::instance().getKey("my-key-id");

// List all keys
auto keys = KeyManager::instance().listKeys();
```

### KeyStore

Persistent key storage with encryption.

```cpp
// Software keystore
SoftwareKeyStore keystore("./keys");
keystore.initialize("master-password");

// Save key
keystore.saveKey("my-key", encryptionKey);

// Load key
auto key = keystore.loadKey("my-key");
```

## Secure Memory API

### SecureBuffer

```cpp
// Create secure buffer
SecureBuffer buffer(32);  // 32 bytes

// With initial data
std::vector<uint8_t> data = {1, 2, 3};
SecureBuffer buffer2(data.data(), data.size());

// Access data
buffer[0] = 0x42;
auto size = buffer.size();

// Automatically zeroed on destruction
```

### SecureString

```cpp
// Create secure string
SecureString password("MySecurePassword");

// Access
auto length = password.length();
auto cstr = password.c_str();

// Compare
if (password == "MySecurePassword") {
    // Match
}

// Automatically zeroed on destruction
```

## File Operations API

```cpp
// Read file
auto data = FileOperations::readFile("input.txt");

// Write file
FileOperations::writeFile("output.txt", data.value());

// Secure delete
FileOperations::secureDelete("sensitive.txt", 7);  // 7-pass

// List files
auto files = FileOperations::listFiles("./", "*.txt", true);

// Stream large files
FileOperations::streamFile("large.bin", 4*1024*1024,
    [](const auto& chunk, size_t offset, size_t total) {
        // Process chunk
        return Result<void>();
    }
);
```

## Result Type

All operations return `Result<T>` for error handling without exceptions:

```cpp
auto result = someOperation();

if (result.isOk()) {
    auto value = result.value();
    // Use value
} else {
    auto error = result.error();
    std::cerr << "Error: " << error.message << std::endl;
}

// Or use valueOr
auto value = result.valueOr(defaultValue);
```

## Configuration API

```cpp
// Get configuration instance
auto& config = Config::instance();

// Load from file
config.load("balance.conf");

// Get values
std::string algo = config.get("default_algorithm", "balance");
int iterations = config.getInt("pbkdf2_iterations", 100000);
bool fips = config.getBool("enable_fips", false);

// Set values
config.set("default_algorithm", "aes256");

// Save to file
config.save("balance.conf");
```

## Logging API

```cpp
// Configure logger
Logger::instance().setLogLevel(LogLevel::Info);
Logger::instance().setLogFile("balance.log");

// Log messages
LOG_DEBUG("Debug message");
LOG_INFO("Info message");
LOG_WARNING("Warning message");
LOG_ERROR("Error message");
LOG_CRITICAL("Critical message");
```

## Chat API

```cpp
// Initialize chat manager
ChatManager::instance().initialize("./balance.db");

// Send message
ChatManager::instance().sendMessage("Hello, World!");

// Get messages
auto messages = ChatManager::instance().getMessages(100, 0);

// Open chat window
ChatManager::instance().openChatWindow();
```

## CLI Integration API

```cpp
// Register custom command
CommandExecutor::instance().registerCommand(
    "mycommand",
    [](const Command& cmd) -> Result<std::string> {
        // Command implementation
        return "Success";
    },
    "My custom command description"
);

// Parse command
auto cmd = CommandParser::parse("encrypt file.txt --algorithm aes256");

// Execute command
auto result = CommandExecutor::instance().execute(cmd.value());
```

## Security API

### FIPS Module

```cpp
// Initialize FIPS mode
auto result = FIPSModule::initialize(true);

// Check FIPS status
if (FIPSModule::isFIPSEnabled()) {
    // FIPS mode active
}

// Self-tests
auto passed = FIPSModule::performSelfTests();
```

### Tamper Detection

```cpp
// Calculate file hash
auto hash = TamperDetection::calculateFileHash("file.txt");

// Verify integrity
auto valid = TamperDetection::verifyFileIntegrity("file.txt", expectedHash);

// Check for debugger
if (TamperDetection::isDebuggerAttached()) {
    // Debugger detected
}

// Log security event
TamperDetection::logSecurityEvent("Suspicious activity", 8);
```

## Thread Safety

All singletons and shared resources are thread-safe:

- `EncryptionRegistry` - Thread-safe singleton
- `KeyManager` - Thread-safe operations with mutex
- `Config` - Thread-safe configuration access
- `Logger` - Thread-safe logging
- `DatabaseManager` - Thread-safe database operations

## Error Handling

Best practices for error handling:

```cpp
// Check for errors
auto result = operation();
if (result.isError()) {
    LOG_ERROR("Operation failed: " + result.error().message);
    return;
}

// Use value
auto value = result.value();

// Chain operations
auto result1 = operation1();
if (result1.isError()) return result1;

auto result2 = operation2(result1.value());
if (result2.isError()) return result2;

return result2;
```

## Example Application

Complete example:

```cpp
#include "balance/Balance.h"

int main() {
    using namespace Balance;

    // Initialize
    SecureMemory::initialize();
    FIPSModule::initialize(false);

    // Generate key
    auto key = KeyManager::instance().generateKey(
        AlgorithmType::Balance,
        "my-app-key"
    );

    // Encrypt file
    auto result = BalanceFormat::encryptFile(
        "document.pdf",
        "document.pdf.balance",
        "SecurePassword123!"
    );

    if (result.isOk()) {
        LOG_INFO("Encryption successful");
    } else {
        LOG_ERROR("Encryption failed: " + result.error().message);
    }

    // Cleanup
    SecureMemory::shutdown();
    return 0;
}
```

## Building Against Balance

### CMakeLists.txt

```cmake
find_package(Balance REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE Balance::Balance)
```

### Compiler Flags

```bash
g++ -std=c++20 -I/usr/include/balance myapp.cpp -lbalance -lssl -lcrypto
```

## API Reference

For complete API reference, see header files in `include/balance/`:

- `Balance.h` - Main public API
- `Types.h` - Type definitions and Result<T>
- Individual module headers in `src/`

## Support

For API questions and support:
- Documentation: https://balance-crypto.org/docs
- Examples: https://github.com/balance/balance-examples
- Issues: https://github.com/balance/balance/issues
