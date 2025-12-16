/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef MEMORY_PROTECTION_H
#define MEMORY_PROTECTION_H

#include <cstddef>
#include <QtGlobal>

/// Cross-platform memory protection utilities
/// Provides secure memory handling to prevent sensitive data from being
/// swapped to disk or remaining in memory after use.
class MemoryProtection
{
public:
    /// Lock a memory region to prevent it from being swapped to disk
    /// On Windows: Uses VirtualLock
    /// On Unix: Uses mlock
    /// @param ptr Pointer to memory region
    /// @param size Size of memory region in bytes
    /// @return true on success, false on failure
    static bool lockMemory(void* ptr, size_t size);

    /// Unlock a previously locked memory region
    /// On Windows: Uses VirtualUnlock
    /// On Unix: Uses munlock
    /// @param ptr Pointer to memory region
    /// @param size Size of memory region in bytes
    /// @return true on success, false on failure
    static bool unlockMemory(void* ptr, size_t size);

    /// Securely erase memory (prevents compiler optimization)
    /// Uses OpenSSL_cleanse if available, otherwise volatile pointer technique
    /// @param ptr Pointer to memory to erase
    /// @param size Size of memory region in bytes
    static void secureErase(void* ptr, size_t size);

    /// Check if memory locking is supported on this platform
    /// @return true if mlock/VirtualLock is available
    static bool isMemoryLockingSupported();

private:
    // Private helper for volatile memset (fallback if OpenSSL not available)
    static void volatileMemset(volatile void* ptr, int value, size_t size);
};

/// RAII wrapper for secure memory allocation
/// Automatically locks memory on construction and unlocks/erases on destruction
template<typename T>
class SecureMemory
{
public:
    explicit SecureMemory(size_t count = 1)
        : m_data(new T[count])
        , m_size(count * sizeof(T))
        , m_locked(false)
    {
        // Lock the memory to prevent swapping
        m_locked = MemoryProtection::lockMemory(m_data, m_size);
    }

    ~SecureMemory()
    {
        if (m_data) {
            // Securely erase the data
            MemoryProtection::secureErase(m_data, m_size);

            // Unlock if it was locked
            if (m_locked) {
                MemoryProtection::unlockMemory(m_data, m_size);
            }

            delete[] m_data;
        }
    }

    // Disable copying
    SecureMemory(const SecureMemory&) = delete;
    SecureMemory& operator=(const SecureMemory&) = delete;

    // Enable moving
    SecureMemory(SecureMemory&& other) noexcept
        : m_data(other.m_data)
        , m_size(other.m_size)
        , m_locked(other.m_locked)
    {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_locked = false;
    }

    SecureMemory& operator=(SecureMemory&& other) noexcept
    {
        if (this != &other) {
            // Clean up existing data
            if (m_data) {
                MemoryProtection::secureErase(m_data, m_size);
                if (m_locked) {
                    MemoryProtection::unlockMemory(m_data, m_size);
                }
                delete[] m_data;
            }

            // Move from other
            m_data = other.m_data;
            m_size = other.m_size;
            m_locked = other.m_locked;

            other.m_data = nullptr;
            other.m_size = 0;
            other.m_locked = false;
        }
        return *this;
    }

    T* data() { return m_data; }
    const T* data() const { return m_data; }

    T& operator[](size_t index) { return m_data[index]; }
    const T& operator[](size_t index) const { return m_data[index]; }

    size_t sizeBytes() const { return m_size; }
    bool isLocked() const { return m_locked; }

private:
    T* m_data;
    size_t m_size;
    bool m_locked;
};

#endif // MEMORY_PROTECTION_H
