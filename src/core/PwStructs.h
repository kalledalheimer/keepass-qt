/*
  KeePass Password Safe - Qt Port
  KDB File Format Data Structures
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef PWSTRUCTS_H
#define PWSTRUCTS_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>

/**
 * @file PwStructs.h
 * @brief KeePass KDB v1.x file format structures
 *
 * CRITICAL: These structures define the on-disk binary format for KDB v1.x files.
 * They MUST maintain exact byte-level compatibility with the MFC version.
 *
 * Migration Status:
 * - Types: Migrated to Qt types (quint8, quint16, quint32, etc.)
 * - Field names: Kept MFC-style for now (will migrate to camelCase later)
 * - New code: Use Qt types everywhere, convert char* to QString immediately
 *
 * The #pragma pack(1) directive is ESSENTIAL - it ensures structures have no
 * padding bytes, matching the exact binary layout of KDB files.
 */

// Legacy type aliases for compatibility with existing code
// TODO: Gradually migrate all code to use quint8/quint16/quint32 directly
using BYTE = quint8;
using USHORT = quint16;
using WORD = quint16;
using DWORD = quint32;
using QWORD = quint64;

// Pack structures to 1-byte alignment for exact binary layout
#pragma pack(push, 1)

/**
 * @struct PW_TIME
 * @brief Time structure - exactly 7 bytes
 *
 * MUST be exactly 7 bytes to match KDB file format.
 * Fields stored in little-endian on disk.
 */
typedef struct _PW_TIME
{
    USHORT shYear;   ///< Year (e.g., 2025)
    BYTE btMonth;    ///< Month (1-12, where 1 = January)
    BYTE btDay;      ///< Day (1-31)
    BYTE btHour;     ///< Hour (0-23)
    BYTE btMinute;   ///< Minute (0-59)
    BYTE btSecond;   ///< Second (0-59)
} PW_TIME;

/**
 * @struct PW_DBHEADER
 * @brief KDB file header - exactly 124 bytes
 *
 * All KeePass KDB files begin with this structure.
 */
typedef struct _PW_DBHEADER
{
    DWORD dwSignature1;      ///< Must be 0x9AA2D903
    DWORD dwSignature2;      ///< Must be 0xB54BFB65
    DWORD dwFlags;           ///< Encryption flags
    DWORD dwVersion;         ///< 0x00030004

    BYTE aMasterSeed[16];    ///< Seed for final key
    BYTE aEncryptionIV[16];  ///< Encryption IV

    DWORD dwGroups;          ///< Group count
    DWORD dwEntries;         ///< Entry count

    BYTE aContentsHash[32];  ///< SHA-256 hash

    BYTE aMasterSeed2[32];   ///< Key transform seed
    DWORD dwKeyEncRounds;    ///< Transform rounds
} PW_DBHEADER;

/**
 * @struct PW_GROUP
 * @brief Group (folder) structure
 */
typedef struct _PW_GROUP
{
    DWORD uGroupId;          ///< Unique ID
    DWORD uImageId;          ///< Icon index
    char* pszGroupName;      ///< Name (UTF-8, heap-allocated)

    PW_TIME tCreation;       ///< Creation time
    PW_TIME tLastMod;        ///< Modification time
    PW_TIME tLastAccess;     ///< Access time
    PW_TIME tExpire;         ///< Expiration time

    USHORT usLevel;          ///< Tree depth
    DWORD dwFlags;           ///< Flags
} PW_GROUP;

/**
 * @struct PW_ENTRY
 * @brief Password entry structure
 */
typedef struct _PW_ENTRY
{
    BYTE uuid[16];           ///< UUID
    DWORD uGroupId;          ///< Parent group
    DWORD uImageId;          ///< Icon index

    char* pszTitle;          ///< Title (UTF-8)
    char* pszURL;            ///< URL (UTF-8)
    char* pszUserName;       ///< Username (UTF-8)

    DWORD uPasswordLen;      ///< Password length
    char* pszPassword;       ///< Password (UTF-8, may be XOR-encrypted)

    char* pszAdditional;     ///< Notes (UTF-8)

    PW_TIME tCreation;       ///< Creation time
    PW_TIME tLastMod;        ///< Modification time
    PW_TIME tLastAccess;     ///< Access time
    PW_TIME tExpire;         ///< Expiration time

    char* pszBinaryDesc;     ///< Attachment description
    BYTE* pBinaryData;       ///< Attachment data
    DWORD uBinaryDataLen;    ///< Attachment size
} PW_ENTRY;

/**
 * @struct PW_UUID_STRUCT
 * @brief UUID wrapper
 */
typedef struct _PW_UUID_STRUCT
{
    BYTE uuid[16];
} PW_UUID_STRUCT;

/**
 * @struct PWDB_REPAIR_INFO
 * @brief Database repair information
 */
typedef struct _PWDB_REPAIR_INFO
{
    DWORD dwOriginalGroupCount;
    DWORD dwOriginalEntryCount;
    DWORD dwRecognizedMetaStreamCount;
} PWDB_REPAIR_INFO;

#pragma pack(pop)

//////////////////////////////////////////////////////////////////////////////
// Plugin structures

#pragma pack(push, 1)

typedef struct
{
    DWORD dwFlags;
    DWORD dwState;
    DWORD dwIcon;
    char* lpCommandString;
    DWORD dwCommandID;
    QWORD dwReserved;
} KP_MENU_ITEM;

typedef struct
{
    const void* pOriginalEntry;
    BYTE uuid[16];
    DWORD uGroupIndex;
    DWORD uImageId;
    const char* lpTitle;
    const char* lpURL;
    const char* lpUserName;
    const char* lpPassword;
    const char* lpAdditional;
    QWORD dwReserved;
} KP_ENTRY;

typedef struct _KP_KEYPROV_INFO
{
    DWORD dwFlags;
    const char* lpName;
    DWORD dwImageIndex;
    QWORD dwReserved;
} KP_KEYPROV_INFO;

typedef struct _KP_KEYPROV_KEY
{
    DWORD dwType;
    DWORD dwFormat;
    void* lpData;
    DWORD dwDataSize;
    QWORD dwReserved;
} KP_KEYPROV_KEY;

typedef struct _KP_KEYPROV_CONTEXT
{
    DWORD dwSize;
    const char* lpProviderName;
    bool bCreatingNewKey;
    bool bConfirming;
    bool bChanging;
    const char* lpDescriptiveName;
} KP_KEYPROV_CONTEXT;

#pragma pack(pop)

//////////////////////////////////////////////////////////////////////////////
// Qt-Native structures (NOT part of binary file format)

/// Meta-stream for custom data
struct PwMetaStream
{
    QString strName;
    QByteArray vData;
};

/// Custom key-value pair
struct CustomKvp
{
    QString key;
    QString value;
};

/// Simple UI state
#pragma pack(push, 1)
typedef struct _PMS_SIMPLE_UI_STATE
{
    DWORD uLastSelectedGroupId;
    DWORD uLastTopVisibleGroupId;
    BYTE aLastSelectedEntryUuid[16];
    BYTE aLastTopVisibleEntryUuid[16];
    DWORD dwReserved01;
    DWORD dwReserved02;
    DWORD dwReserved03;
    DWORD dwReserved04;
    DWORD dwReserved05;
    DWORD dwReserved06;
    DWORD dwReserved07;
    DWORD dwReserved08;
    DWORD dwReserved09;
    DWORD dwReserved10;
    DWORD dwReserved11;
    DWORD dwReserved12;
    DWORD dwReserved13;
    DWORD dwReserved14;
    DWORD dwReserved15;
    DWORD dwReserved16;
} PMS_SIMPLE_UI_STATE;
#pragma pack(pop)

//////////////////////////////////////////////////////////////////////////////
// Debug validation

#ifdef QT_DEBUG
#define ASSERT_ENTRY(pp) \
    Q_ASSERT((pp) != nullptr); \
    Q_ASSERT((pp)->pszTitle != nullptr); \
    Q_ASSERT((pp)->pszUserName != nullptr); \
    Q_ASSERT((pp)->pszURL != nullptr); \
    Q_ASSERT((pp)->pszPassword != nullptr); \
    Q_ASSERT((pp)->pszAdditional != nullptr); \
    Q_ASSERT((pp)->pszBinaryDesc != nullptr); \
    if (((pp)->uBinaryDataLen != 0) && ((pp)->pBinaryData == nullptr)) { \
        Q_ASSERT(false); \
    }
#else
#define ASSERT_ENTRY(pp)
#endif

#endif // PWSTRUCTS_H
