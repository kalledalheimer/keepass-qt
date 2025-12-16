/*
  KeePass Password Safe - Qt Port
  System Definitions - Compatibility header for MFC crypto code
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef SYS_DEF_EX_H
#define SYS_DEF_EX_H

#include <QtGlobal>
#include "PwStructs.h"

// Platform-independent type definitions for crypto code
// These ensure the MFC crypto implementations work unchanged

#ifndef UINT8
typedef quint8  UINT8;
#endif

#ifndef UINT16
typedef quint16 UINT16;
#endif

#ifndef UINT32
typedef quint32 UINT32;
#endif

#ifndef UINT64
typedef quint64 UINT64;
#endif

#ifndef INT8
typedef qint8   INT8;
#endif

#ifndef INT16
typedef qint16  INT16;
#endif

#ifndef INT32
typedef qint32  INT32;
#endif

#ifndef INT64
typedef qint64  INT64;
#endif

// Assertion macro
#ifdef QT_DEBUG
    #define ASSERT Q_ASSERT
    #define VERIFY Q_ASSERT
#else
    #define ASSERT(x) ((void)0)
    #define VERIFY(x) (x)
#endif

// NULL definition
#ifndef NULL
    #define NULL nullptr
#endif

// Boolean values (for compatibility with MFC code)
#ifndef TRUE
    #define TRUE  1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#endif // SYS_DEF_EX_H
