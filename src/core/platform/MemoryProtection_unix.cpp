/*
  Qt KeePass - Memory Protection (Unix/macOS/Linux)

  Implementation using mlock/munlock system calls.
*/

#include "MemoryProtection.h"
#include <cstring>
#include <new>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <openssl/crypto.h>  // For OPENSSL_cleanse

// Platform-specific implementation

bool MemoryProtection::lock(void* addr, size_t len)
{
    if (!addr || len == 0) {
        return false;
    }

    // mlock returns 0 on success, -1 on error
    return mlock(addr, len) == 0;
}

bool MemoryProtection::unlock(void* addr, size_t len)
{
    if (!addr || len == 0) {
        return false;
    }

    // munlock returns 0 on success, -1 on error
    return munlock(addr, len) == 0;
}

bool MemoryProtection::isAvailable(size_t len)
{
    // Check if we have permission to lock memory
    // On macOS/Linux, this requires appropriate privileges or RLIMIT_MEMLOCK

    struct rlimit limit;
    if (getrlimit(RLIMIT_MEMLOCK, &limit) != 0) {
        return false;  // Can't query limit
    }

    // Check if requested size exceeds limit
    if (len > 0 && limit.rlim_cur != RLIM_INFINITY && len > limit.rlim_cur) {
        return false;
    }

    return true;
}

size_t MemoryProtection::getPageSize()
{
    return static_cast<size_t>(sysconf(_SC_PAGESIZE));
}

size_t MemoryProtection::getMaxLockableMemory()
{
    struct rlimit limit;
    if (getrlimit(RLIMIT_MEMLOCK, &limit) != 0) {
        return 0;  // Unknown
    }

    if (limit.rlim_cur == RLIM_INFINITY) {
        return 0;  // Unlimited
    }

    return limit.rlim_cur;
}

// SecureMemory implementation

SecureMemory::SecureMemory(size_t size)
    : m_data(nullptr)
    , m_size(size)
    , m_locked(false)
{
    if (size == 0) {
        return;
    }

    // Allocate memory
    m_data = ::operator new(size);
    if (!m_data) {
        throw std::bad_alloc();
    }

    // Zero initialize
    std::memset(m_data, 0, size);

    // Try to lock (failure is not fatal, just less secure)
    m_locked = MemoryProtection::lock(m_data, size);

    // Note: If locking fails, we still use the memory
    // It's just not protected from swapping
}

SecureMemory::~SecureMemory()
{
    clear();

    if (m_data) {
        // Unlock if locked
        if (m_locked) {
            MemoryProtection::unlock(m_data, m_size);
            m_locked = false;
        }

        // Free memory
        ::operator delete(m_data);
        m_data = nullptr;
    }

    m_size = 0;
}

SecureMemory::SecureMemory(SecureMemory&& other) noexcept
    : m_data(other.m_data)
    , m_size(other.m_size)
    , m_locked(other.m_locked)
{
    other.m_data = nullptr;
    other.m_size = 0;
    other.m_locked = false;
}

SecureMemory& SecureMemory::operator=(SecureMemory&& other) noexcept
{
    if (this != &other) {
        // Clean up current resources
        clear();
        if (m_data && m_locked) {
            MemoryProtection::unlock(m_data, m_size);
        }
        if (m_data) {
            ::operator delete(m_data);
        }

        // Transfer ownership
        m_data = other.m_data;
        m_size = other.m_size;
        m_locked = other.m_locked;

        other.m_data = nullptr;
        other.m_size = 0;
        other.m_locked = false;
    }
    return *this;
}

void SecureMemory::clear()
{
    if (m_data && m_size > 0) {
        // Use OpenSSL's secure memory clearing
        OPENSSL_cleanse(m_data, m_size);
    }
}
