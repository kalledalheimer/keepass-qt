/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "MemoryProtection.h"
#include <cstring>

// OpenSSL for secure memory erasure
#include <openssl/crypto.h>

// Platform-specific includes
#ifdef Q_OS_WIN
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
    #include <errno.h>
#endif

bool MemoryProtection::lockMemory(void* ptr, size_t size)
{
    if (!ptr || size == 0)
        return false;

#ifdef Q_OS_WIN
    // Windows: Use VirtualLock
    return VirtualLock(ptr, size) != 0;
#else
    // Unix (macOS, Linux): Use mlock
    // Note: May require elevated privileges or ulimit adjustment
    return mlock(ptr, size) == 0;
#endif
}

bool MemoryProtection::unlockMemory(void* ptr, size_t size)
{
    if (!ptr || size == 0)
        return false;

#ifdef Q_OS_WIN
    // Windows: Use VirtualUnlock
    return VirtualUnlock(ptr, size) != 0;
#else
    // Unix (macOS, Linux): Use munlock
    return munlock(ptr, size) == 0;
#endif
}

void MemoryProtection::secureErase(void* ptr, size_t size)
{
    if (!ptr || size == 0)
        return;

    // Use OpenSSL's secure memory clearing function
    // This is designed to resist compiler optimization that might remove
    // the memset call if the memory is about to be freed
    OPENSSL_cleanse(ptr, size);

    // Additional safeguard: explicit memory barrier
    // This prevents the compiler from reordering operations
    #ifdef __GNUC__
        __asm__ __volatile__("" ::: "memory");
    #elif defined(_MSC_VER)
        _ReadWriteBarrier();
    #endif
}

bool MemoryProtection::isMemoryLockingSupported()
{
#ifdef Q_OS_WIN
    // VirtualLock is always available on Windows
    return true;
#else
    // Check if mlock is available (it is on all modern Unix systems)
    // We don't test if we have permission, just if the function exists
    #ifdef _POSIX_MEMLOCK
        return _POSIX_MEMLOCK > 0;
    #else
        // Try to lock a small page and see if it works
        size_t pageSize = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        if (pageSize == static_cast<size_t>(-1))
            pageSize = 4096; // Fallback to common page size

        void* testPtr = aligned_alloc(pageSize, pageSize);
        if (!testPtr)
            return false;

        bool canLock = (mlock(testPtr, pageSize) == 0);

        if (canLock)
            munlock(testPtr, pageSize);

        free(testPtr);
        return canLock;
    #endif
#endif
}

void MemoryProtection::volatileMemset(volatile void* ptr, int value, size_t size)
{
    // Fallback method if OpenSSL_cleanse is not available
    // The volatile keyword prevents compiler optimization
    volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
    while (size--)
        *p++ = static_cast<unsigned char>(value);
}
