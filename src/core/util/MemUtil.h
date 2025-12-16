/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef MEM_UTIL_H
#define MEM_UTIL_H

#include <cstddef>
#include <cstring>

/// Memory utility functions for secure operations
/// Provides compatibility with MFC version's mem_erase function
namespace MemUtil
{
    /// Securely erase memory (wrapper for MemoryProtection::secureErase)
    /// This function name matches the MFC version for easy porting
    /// @param ptr Pointer to memory to erase
    /// @param size Size of memory region in bytes
    inline void mem_erase(void* ptr, size_t size)
    {
        if (!ptr || size == 0)
            return;

        // Forward to MemoryProtection (which uses OPENSSL_cleanse)
        extern void secureEraseMemory(void* ptr, size_t size);
        secureEraseMemory(ptr, size);
    }

    /// Zero-fill memory
    /// @param ptr Pointer to memory
    /// @param size Size of memory region in bytes
    inline void mem_zero(void* ptr, size_t size)
    {
        if (ptr && size > 0)
            std::memset(ptr, 0, size);
    }

    /// Compare memory regions in constant time (timing attack resistant)
    /// @param ptr1 First memory region
    /// @param ptr2 Second memory region
    /// @param size Size to compare
    /// @return true if equal, false otherwise
    bool mem_equals_const_time(const void* ptr1, const void* ptr2, size_t size);

    /// Allocate aligned memory
    /// @param alignment Alignment requirement (must be power of 2)
    /// @param size Size to allocate
    /// @return Pointer to aligned memory, or nullptr on failure
    void* mem_alloc_aligned(size_t alignment, size_t size);

    /// Free aligned memory allocated with mem_alloc_aligned
    /// @param ptr Pointer to free
    void mem_free_aligned(void* ptr);
}

#endif // MEM_UTIL_H
