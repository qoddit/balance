# Test configuration
cmake_minimum_required(VERSION 3.20)

# Download and configure Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.12.1.tar.gz
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test sources
set(TEST_SOURCES
    test_main.cpp
    core/test_aes.cpp
    core/test_balance_format.cpp
    cli/test_command_parser.cpp
    security/test_secure_memory.cpp
)

# Create test executable
add_executable(balance_tests ${TEST_SOURCES})

# Include directories
target_include_directories(balance_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(balance_tests PRIVATE
    gtest
    gtest_main
    OpenSSL::SSL
    OpenSSL::Crypto
    SQLite::SQLite3
)

# Link with main project sources (excluding main.cpp)
target_sources(balance_tests PRIVATE
    ${CORE_ENCRYPTION_SOURCES}
    ${CORE_SECURITY_SOURCES}
    ${CORE_STORAGE_SOURCES}
    ${CLI_SOURCES}
    ${CHAT_SOURCES}
    ${UTILS_SOURCES}
)

# Add tests to CTest
add_test(NAME balance_tests COMMAND balance_tests)

# Copy test data files
file(COPY test_data DESTINATION ${CMAKE_BINARY_DIR}/tests)
