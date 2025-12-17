/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "MemUtil.h"
#include "../crypto/MemoryProtection.h"
#include <cstdlib>

namespace MemUtil {

// External function called by mem_erase
void secureEraseMemory(void* ptr, size_t size)
{
    MemoryProtection::secureErase(ptr, size);
}

} // namespace MemUtil

bool MemUtil::mem_equals_const_time(const void* ptr1, const void* ptr2, size_t size)
{
    if (!ptr1 || !ptr2)
        return (ptr1 == ptr2);

    if (size == 0)
        return true;

    const unsigned char* p1 = static_cast<const unsigned char*>(ptr1);
    const unsigned char* p2 = static_cast<const unsigned char*>(ptr2);

    // Use constant-time comparison to prevent timing attacks
    // XOR all bytes and accumulate the result
    // If any byte differs, diff will be non-zero
    unsigned char diff = 0;
    for (size_t i = 0; i < size; ++i) {
        diff |= (p1[i] ^ p2[i]);
    }

    return (diff == 0);
}

void* MemUtil::mem_alloc_aligned(size_t alignment, size_t size)
{
    if (size == 0)
        return nullptr;

    // Ensure alignment is a power of 2
    if (alignment == 0 || (alignment & (alignment - 1)) != 0)
        return nullptr;

#ifdef Q_OS_WIN
    // Windows: Use _aligned_malloc
    return _aligned_malloc(size, alignment);
#else
    // POSIX: Use aligned_alloc (C11) or posix_memalign
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        return aligned_alloc(alignment, size);
    #else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, alignment, size) == 0)
            return ptr;
        return nullptr;
    #endif
#endif
}

void MemUtil::mem_free_aligned(void* ptr)
{
    if (!ptr)
        return;

#ifdef Q_OS_WIN
    // Windows: Use _aligned_free
    _aligned_free(ptr);
#else
    // POSIX: Use regular free
    free(ptr);
#endif
}
