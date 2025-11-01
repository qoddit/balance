cmake_minimum_required(VERSION 3.20)
project(Balance VERSION 1.0.0 LANGUAGES CXX)

# C++20 standard required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Qml Quick)
find_package(OpenSSL 3.0 REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)

# Compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0 -Wall -Wextra -Wpedantic)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-flto)
        add_link_options(-flto)
    endif()
endif()

# Security hardening flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fstack-protector-strong -D_FORTIFY_SOURCE=2)
    add_compile_options(-Wformat -Wformat-security)
endif()

# Source files - Core Encryption
set(CORE_ENCRYPTION_SOURCES
    src/core/encryption/EncryptionEngine.cpp
    src/core/encryption/BalanceFormat.cpp
    src/core/encryption/AESEncryption.cpp
    src/core/encryption/RSAEncryption.cpp
    src/core/encryption/SerpentEncryption.cpp
    src/core/encryption/BlowfishEncryption.cpp
    src/core/encryption/ChaCha20Encryption.cpp
    src/core/encryption/TwofishEncryption.cpp
    src/core/encryption/CAST256Encryption.cpp
)

# Source files - Core Security
set(CORE_SECURITY_SOURCES
    src/core/security/SecureMemory.cpp
    src/core/security/KeyManager.cpp
    src/core/security/TamperDetection.cpp
    src/core/security/FIPSModule.cpp
)

# Source files - Core Storage
set(CORE_STORAGE_SOURCES
    src/core/storage/FileOperations.cpp
    src/core/storage/KeyStore.cpp
    src/core/storage/DatabaseManager.cpp
)

# Source files - CLI
set(CLI_SOURCES
    src/cli/CommandParser.cpp
    src/cli/CommandExecutor.cpp
    src/cli/CommandHistory.cpp
    src/cli/Autocomplete.cpp
    src/cli/commands/BasicCommands.cpp
)

# Source files - GUI
set(GUI_SOURCES
    src/gui/MainWindow.cpp
)

# Source files - Chat
set(CHAT_SOURCES
    src/chat/ChatManager.cpp
    src/chat/MessageStore.cpp
    src/chat/LocalChatProvider.cpp
    src/chat/SupabaseProvider.cpp
)

# Source files - Utils
set(UTILS_SOURCES
    src/utils/Logger.cpp
    src/utils/Config.cpp
    src/utils/Validators.cpp
)

# QML resources
set(QML_RESOURCES
    resources/qml.qrc
)

# Main executable
add_executable(balance
    src/main.cpp
    ${CORE_ENCRYPTION_SOURCES}
    ${CORE_SECURITY_SOURCES}
    ${CORE_STORAGE_SOURCES}
    ${CLI_SOURCES}
    ${GUI_SOURCES}
    ${CHAT_SOURCES}
    ${UTILS_SOURCES}
    ${QML_RESOURCES}
)

# Include directories
target_include_directories(balance PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(balance PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    OpenSSL::SSL
    OpenSSL::Crypto
    SQLite::SQLite3
    CURL::libcurl
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(balance PRIVATE pthread dl)
elseif(WIN32)
    target_link_libraries(balance PRIVATE ws2_32 crypt32)
endif()

# Enable testing
enable_testing()

# Add subdirectory for tests
add_subdirectory(tests)

# Install targets
install(TARGETS balance
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install documentation
install(DIRECTORY docs/
    DESTINATION share/doc/balance
    FILES_MATCHING PATTERN "*.md"
)

# Install resources
install(DIRECTORY resources/
    DESTINATION share/balance/resources
    PATTERN "*.qrc" EXCLUDE
)
