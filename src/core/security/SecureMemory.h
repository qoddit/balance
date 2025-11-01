#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace Balance {

/**
 * Secure memory utilities for handling sensitive data
 * Uses OpenSSL secure memory functions and memory locking
 */
class SecureMemory {
public:
    /**
     * Initialize secure memory subsystem
     * Must be called before using secure memory functions
     */
    static bool initialize();

    /**
     * Shutdown secure memory subsystem
     * Clears and frees all secure memory
     */
    static void shutdown();

    /**
     * Allocate secure memory
     * Memory is locked (not swapped to disk) and cleared on deallocation
     * @param size Size in bytes to allocate
     * @return Pointer to secure memory or nullptr on failure
     */
    static void* allocate(size_t size);

    /**
     * Free secure memory
     * Automatically zeros memory before freeing
     * @param ptr Pointer to secure memory
     */
    static void free(void* ptr);

    /**
     * Zero memory securely
     * Prevents compiler optimization from removing the zeroing
     * @param ptr Pointer to memory
     * @param size Size in bytes
     */
    static void zero(void* ptr, size_t size);

    /**
     * Lock memory pages to prevent swapping
     * @param ptr Pointer to memory
     * @param size Size in bytes
     * @return true on success, false on failure
     */
    static bool lock(void* ptr, size_t size);

    /**
     * Unlock memory pages
     * @param ptr Pointer to memory
     * @param size Size in bytes
     * @return true on success, false on failure
     */
    static bool unlock(void* ptr, size_t size);
};

/**
 * Custom deleter for secure memory
 */
struct SecureDeleter {
    void operator()(void* ptr) const {
        if (ptr) {
            SecureMemory::free(ptr);
        }
    }
};

/**
 * Secure buffer for storing sensitive binary data
 * Automatically zeros memory on destruction
 */
class SecureBuffer {
public:
    SecureBuffer();
    explicit SecureBuffer(size_t size);
    SecureBuffer(const uint8_t* data, size_t size);
    ~SecureBuffer();

    // Delete copy constructor and assignment
    SecureBuffer(const SecureBuffer&) = delete;
    SecureBuffer& operator=(const SecureBuffer&) = delete;

    // Allow move
    SecureBuffer(SecureBuffer&& other) noexcept;
    SecureBuffer& operator=(SecureBuffer&& other) noexcept;

    /**
     * Get pointer to buffer data
     */
    uint8_t* data() { return buffer; }
    const uint8_t* data() const { return buffer; }

    /**
     * Get buffer size
     */
    size_t size() const { return bufferSize; }

    /**
     * Resize buffer (copies existing data)
     */
    bool resize(size_t newSize);

    /**
     * Clear buffer (zero all data)
     */
    void clear();

    /**
     * Check if buffer is empty
     */
    bool empty() const { return bufferSize == 0 || buffer == nullptr; }

    /**
     * Convert to std::vector (creates a copy)
     */
    std::vector<uint8_t> toVector() const;

    /**
     * Array access operators
     */
    uint8_t& operator[](size_t index) { return buffer[index]; }
    const uint8_t& operator[](size_t index) const { return buffer[index]; }

private:
    uint8_t* buffer;
    size_t bufferSize;

    void cleanup();
};

/**
 * Secure string for storing sensitive text data (passwords, keys)
 * Automatically zeros memory on destruction
 */
class SecureString {
public:
    SecureString();
    explicit SecureString(const std::string& str);
    explicit SecureString(const char* str);
    SecureString(const char* str, size_t length);
    ~SecureString();

    // Delete copy constructor and assignment
    SecureString(const SecureString&) = delete;
    SecureString& operator=(const SecureString&) = delete;

    // Allow move
    SecureString(SecureString&& other) noexcept;
    SecureString& operator=(SecureString&& other) noexcept;

    /**
     * Get C-string pointer
     */
    const char* c_str() const { return buffer; }

    /**
     * Get string length
     */
    size_t length() const { return stringLength; }

    /**
     * Get string size (same as length)
     */
    size_t size() const { return stringLength; }

    /**
     * Check if string is empty
     */
    bool empty() const { return stringLength == 0 || buffer == nullptr; }

    /**
     * Clear string (zero all data)
     */
    void clear();

    /**
     * Convert to std::string (creates a copy)
     * WARNING: The copy is not secure
     */
    std::string toString() const;

    /**
     * Append data to string
     */
    bool append(const char* str);
    bool append(const std::string& str);

    /**
     * Array access operator
     */
    char& operator[](size_t index) { return buffer[index]; }
    const char& operator[](size_t index) const { return buffer[index]; }

    /**
     * Comparison operators
     */
    bool operator==(const SecureString& other) const;
    bool operator==(const std::string& other) const;
    bool operator==(const char* other) const;

private:
    char* buffer;
    size_t stringLength;
    size_t capacity;

    void cleanup();
    bool reserve(size_t newCapacity);
};

/**
 * RAII helper for automatic memory cleanup
 */
template<typename T>
class SecureScope {
public:
    explicit SecureScope(T* ptr) : pointer(ptr) {}
    ~SecureScope() {
        if (pointer) {
            SecureMemory::zero(pointer, sizeof(T));
        }
    }

    T* get() { return pointer; }
    const T* get() const { return pointer; }

    T* operator->() { return pointer; }
    const T* operator->() const { return pointer; }

    T& operator*() { return *pointer; }
    const T& operator*() const { return *pointer; }

private:
    T* pointer;
};

} // namespace Balance
