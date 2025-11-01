#include "SecureMemory.h"
#include <openssl/crypto.h>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Balance {

// SecureMemory static methods

bool SecureMemory::initialize() {
    // Initialize OpenSSL secure memory (64KB default)
    if (CRYPTO_secure_malloc_init(65536, 32) == 0) {
        return false;
    }
    return true;
}

void SecureMemory::shutdown() {
    CRYPTO_secure_malloc_done();
}

void* SecureMemory::allocate(size_t size) {
    if (size == 0) {
        return nullptr;
    }

    // Try OpenSSL secure malloc first
    void* ptr = OPENSSL_secure_malloc(size);
    if (ptr) {
        return ptr;
    }

    // Fallback to regular malloc with manual locking
    ptr = std::malloc(size);
    if (ptr) {
        lock(ptr, size);
        std::memset(ptr, 0, size);
    }

    return ptr;
}

void SecureMemory::free(void* ptr) {
    if (!ptr) {
        return;
    }

    // Check if it's OpenSSL secure memory
    if (OPENSSL_secure_actual_size(ptr) > 0) {
        OPENSSL_secure_clear_free(ptr, OPENSSL_secure_actual_size(ptr));
    } else {
        // Fallback: zero and free regular memory
        // Note: We don't know the size, so we can't zero it properly here
        // This is why SecureBuffer and SecureString track their own sizes
        std::free(ptr);
    }
}

void SecureMemory::zero(void* ptr, size_t size) {
    if (!ptr || size == 0) {
        return;
    }

    // Use OpenSSL's secure zero function (prevents compiler optimization)
    OPENSSL_cleanse(ptr, size);
}

bool SecureMemory::lock(void* ptr, size_t size) {
    if (!ptr || size == 0) {
        return false;
    }

#ifdef _WIN32
    // Windows: Use VirtualLock
    return VirtualLock(ptr, size) != 0;
#else
    // Unix/Linux: Use mlock
    return mlock(ptr, size) == 0;
#endif
}

bool SecureMemory::unlock(void* ptr, size_t size) {
    if (!ptr || size == 0) {
        return false;
    }

#ifdef _WIN32
    // Windows: Use VirtualUnlock
    return VirtualUnlock(ptr, size) != 0;
#else
    // Unix/Linux: Use munlock
    return munlock(ptr, size) == 0;
#endif
}

// SecureBuffer implementation

SecureBuffer::SecureBuffer()
    : buffer(nullptr), bufferSize(0) {
}

SecureBuffer::SecureBuffer(size_t size)
    : buffer(nullptr), bufferSize(0) {
    if (size > 0) {
        buffer = static_cast<uint8_t*>(SecureMemory::allocate(size));
        if (buffer) {
            bufferSize = size;
        }
    }
}

SecureBuffer::SecureBuffer(const uint8_t* data, size_t size)
    : SecureBuffer(size) {
    if (buffer && data) {
        std::memcpy(buffer, data, size);
    }
}

SecureBuffer::~SecureBuffer() {
    cleanup();
}

SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept
    : buffer(other.buffer), bufferSize(other.bufferSize) {
    other.buffer = nullptr;
    other.bufferSize = 0;
}

SecureBuffer& SecureBuffer::operator=(SecureBuffer&& other) noexcept {
    if (this != &other) {
        cleanup();
        buffer = other.buffer;
        bufferSize = other.bufferSize;
        other.buffer = nullptr;
        other.bufferSize = 0;
    }
    return *this;
}

bool SecureBuffer::resize(size_t newSize) {
    if (newSize == bufferSize) {
        return true;
    }

    if (newSize == 0) {
        clear();
        return true;
    }

    uint8_t* newBuffer = static_cast<uint8_t*>(SecureMemory::allocate(newSize));
    if (!newBuffer) {
        return false;
    }

    // Copy existing data
    if (buffer && bufferSize > 0) {
        size_t copySize = std::min(bufferSize, newSize);
        std::memcpy(newBuffer, buffer, copySize);
    }

    cleanup();
    buffer = newBuffer;
    bufferSize = newSize;

    return true;
}

void SecureBuffer::clear() {
    cleanup();
}

std::vector<uint8_t> SecureBuffer::toVector() const {
    if (!buffer || bufferSize == 0) {
        return {};
    }
    return std::vector<uint8_t>(buffer, buffer + bufferSize);
}

void SecureBuffer::cleanup() {
    if (buffer) {
        SecureMemory::zero(buffer, bufferSize);
        SecureMemory::free(buffer);
        buffer = nullptr;
    }
    bufferSize = 0;
}

// SecureString implementation

SecureString::SecureString()
    : buffer(nullptr), stringLength(0), capacity(0) {
}

SecureString::SecureString(const std::string& str)
    : SecureString(str.c_str(), str.length()) {
}

SecureString::SecureString(const char* str)
    : SecureString(str, str ? std::strlen(str) : 0) {
}

SecureString::SecureString(const char* str, size_t length)
    : buffer(nullptr), stringLength(0), capacity(0) {
    if (str && length > 0) {
        capacity = length + 1;  // +1 for null terminator
        buffer = static_cast<char*>(SecureMemory::allocate(capacity));
        if (buffer) {
            std::memcpy(buffer, str, length);
            buffer[length] = '\0';
            stringLength = length;
        }
    }
}

SecureString::~SecureString() {
    cleanup();
}

SecureString::SecureString(SecureString&& other) noexcept
    : buffer(other.buffer), stringLength(other.stringLength), capacity(other.capacity) {
    other.buffer = nullptr;
    other.stringLength = 0;
    other.capacity = 0;
}

SecureString& SecureString::operator=(SecureString&& other) noexcept {
    if (this != &other) {
        cleanup();
        buffer = other.buffer;
        stringLength = other.stringLength;
        capacity = other.capacity;
        other.buffer = nullptr;
        other.stringLength = 0;
        other.capacity = 0;
    }
    return *this;
}

void SecureString::clear() {
    cleanup();
}

std::string SecureString::toString() const {
    if (!buffer || stringLength == 0) {
        return "";
    }
    return std::string(buffer, stringLength);
}

bool SecureString::append(const char* str) {
    if (!str) {
        return false;
    }
    size_t len = std::strlen(str);
    size_t newLength = stringLength + len;

    if (newLength + 1 > capacity) {
        if (!reserve(newLength + 1)) {
            return false;
        }
    }

    std::memcpy(buffer + stringLength, str, len);
    stringLength = newLength;
    buffer[stringLength] = '\0';

    return true;
}

bool SecureString::append(const std::string& str) {
    return append(str.c_str());
}

bool SecureString::operator==(const SecureString& other) const {
    if (stringLength != other.stringLength) {
        return false;
    }
    if (!buffer && !other.buffer) {
        return true;
    }
    if (!buffer || !other.buffer) {
        return false;
    }
    return std::memcmp(buffer, other.buffer, stringLength) == 0;
}

bool SecureString::operator==(const std::string& other) const {
    if (stringLength != other.length()) {
        return false;
    }
    if (!buffer) {
        return other.empty();
    }
    return std::memcmp(buffer, other.c_str(), stringLength) == 0;
}

bool SecureString::operator==(const char* other) const {
    if (!other) {
        return !buffer || stringLength == 0;
    }
    size_t otherLen = std::strlen(other);
    if (stringLength != otherLen) {
        return false;
    }
    if (!buffer) {
        return otherLen == 0;
    }
    return std::memcmp(buffer, other, stringLength) == 0;
}

void SecureString::cleanup() {
    if (buffer) {
        SecureMemory::zero(buffer, capacity);
        SecureMemory::free(buffer);
        buffer = nullptr;
    }
    stringLength = 0;
    capacity = 0;
}

bool SecureString::reserve(size_t newCapacity) {
    if (newCapacity <= capacity) {
        return true;
    }

    char* newBuffer = static_cast<char*>(SecureMemory::allocate(newCapacity));
    if (!newBuffer) {
        return false;
    }

    // Copy existing data
    if (buffer && stringLength > 0) {
        std::memcpy(newBuffer, buffer, stringLength);
        newBuffer[stringLength] = '\0';
    }

    cleanup();
    buffer = newBuffer;
    capacity = newCapacity;

    return true;
}

} // namespace Balance
