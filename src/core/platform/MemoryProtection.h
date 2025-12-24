/*
  Qt KeePass - Memory Protection Interface

  Cross-platform memory locking to prevent passwords from being swapped to disk.

  Platform implementations:
  - Unix (macOS/Linux): mlock/munlock
  - Windows: VirtualLock/VirtualUnlock
*/

#ifndef MEMORYPROTECTION_H
#define MEMORYPROTECTION_H

#include <cstddef>

class MemoryProtection
{
public:
    /**
     * Lock memory region to prevent swapping to disk
     * @param addr Memory address
     * @param len Length in bytes
     * @return true if successful, false otherwise
     */
    static bool lock(void* addr, size_t len);

    /**
     * Unlock previously locked memory region
     * @param addr Memory address
     * @param len Length in bytes
     * @return true if successful, false otherwise
     */
    static bool unlock(void* addr, size_t len);

    /**
     * Check if memory locking is available on this platform
     * @param len Size to check (some platforms have limits)
     * @return true if locking is available for this size
     */
    static bool isAvailable(size_t len = 0);

    /**
     * Get system page size (for alignment)
     * @return Page size in bytes
     */
    static size_t getPageSize();

    /**
     * Get maximum lockable memory (if available)
     * @return Maximum bytes, or 0 if unlimited/unknown
     */
    static size_t getMaxLockableMemory();
};

/**
 * RAII wrapper for secure memory allocation with automatic locking/unlocking
 *
 * Usage:
 *   SecureMemory mem(1024);  // Allocates and locks 1KB
 *   memcpy(mem.data(), password, passwordLen);
 *   // ... use memory ...
 *   // Destructor automatically clears and unlocks
 */
class SecureMemory
{
public:
    /**
     * Allocate and lock memory
     * @param size Size in bytes
     * @throws std::bad_alloc if allocation fails
     */
    explicit SecureMemory(size_t size);

    /**
     * Destructor: clears, unlocks, and frees memory
     */
    ~SecureMemory();

    // Prevent copying
    SecureMemory(const SecureMemory&) = delete;
    SecureMemory& operator=(const SecureMemory&) = delete;

    // Allow moving
    SecureMemory(SecureMemory&& other) noexcept;
    SecureMemory& operator=(SecureMemory&& other) noexcept;

    /**
     * Access raw memory
     * @return Pointer to locked memory
     */
    void* data() { return m_data; }
    const void* data() const { return m_data; }

    /**
     * Get allocated size
     * @return Size in bytes
     */
    size_t size() const { return m_size; }

    /**
     * Check if memory is currently locked
     * @return true if locked
     */
    bool isLocked() const { return m_locked; }

private:
    void* m_data;
    size_t m_size;
    bool m_locked;

    void clear();
};

#endif // MEMORYPROTECTION_H
