/*
  KeePass Password Safe - Qt Port
  System Definitions - Compatibility header for third-party crypto code
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef SYSDEFEX_H
#define SYSDEFEX_H

#include <QtGlobal>

/**
 * @file SysDefEx.h
 * @brief Compatibility type definitions for third-party cryptographic code
 *
 * This header provides Windows/MFC-style type definitions for the third-party
 * cryptographic implementations (Rijndael, Twofish, SHA2) which are public domain
 * and remain unchanged from the original MFC version.
 *
 * NOTE: New code should NOT use these types. Use Qt types directly:
 * - Use quint8, quint16, quint32, quint64 for unsigned integers
 * - Use qint8, qint16, qint32, qint64 for signed integers
 * - Use bool instead of BOOL
 * - Use true/false instead of TRUE/FALSE
 * - Use nullptr instead of NULL
 *
 * These legacy types exist ONLY for compatibility with the public domain
 * crypto code that we cannot modify.
 */

// Legacy type definitions for crypto code compatibility
#ifndef UINT8
using UINT8 = quint8;
#endif

#ifndef UINT16
using UINT16 = quint16;
#endif

#ifndef UINT32
using UINT32 = quint32;
#endif

#ifndef UINT64
using UINT64 = quint64;
#endif

#ifndef INT8
using INT8 = qint8;
#endif

#ifndef INT16
using INT16 = qint16;
#endif

#ifndef INT32
using INT32 = qint32;
#endif

#ifndef INT64
using INT64 = qint64;
#endif

// Assertion macros
#ifdef QT_DEBUG
    #define ASSERT Q_ASSERT
    #define VERIFY Q_ASSERT
#else
    #define ASSERT(x) ((void)0)
    #define VERIFY(x) (x)
#endif

// Legacy definitions (for third-party crypto code only)
// DO NOT USE in new code
#ifndef NULL
    #define NULL nullptr
#endif

#ifndef TRUE
    #define TRUE  1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

// TCHAR compatibility (for crypto code that uses character arrays)
// New code should use QString directly
#ifndef TCHAR
using TCHAR = char;
#endif

#endif // SYSDEFEX_H
