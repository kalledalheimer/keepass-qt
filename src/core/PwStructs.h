/*
  KeePass Password Safe - Qt Port
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

#ifndef PW_STRUCTS_H
#define PW_STRUCTS_H

#include <QtGlobal>
#include <QString>
#include <QVector>

// Platform-independent type definitions matching Windows types used in KDB format
#ifndef _WIN32
typedef quint8  BYTE;
typedef quint16 USHORT;
typedef quint16 WORD;
typedef quint32 DWORD;
typedef quint64 QWORD;
typedef qint32  BOOL;
typedef quint8  UINT8;
#else
#include <windows.h>
typedef unsigned char UINT8;
#endif

// Pack structures to match exact binary layout of KDB format
#pragma pack(push, 1)

/// Time structure - MUST maintain exact 7-byte layout for KDB compatibility
typedef struct _PW_TIME
{
    USHORT shYear;  ///< Year. 2004 means 2004.
    BYTE btMonth;   ///< Month. Ranges from 1 = Jan to 12 = Dec.
    BYTE btDay;     ///< Day. The first day is 1.
    BYTE btHour;    ///< Hour. Begins with hour 0, maximum value is 23.
    BYTE btMinute;  ///< Minutes. Begins at 0, maximum value is 59.
    BYTE btSecond;  ///< Seconds. Begins at 0, maximum value is 59.
} PW_TIME;

/// Database header structure - MUST maintain exact 124-byte layout
/// All KeePass KDB files begin with this structure
typedef struct _PW_DBHEADER
{
    DWORD dwSignature1;      ///< File identifier, must be PWM_DBSIG_1 (0x9AA2D903)
    DWORD dwSignature2;      ///< File identifier, must be PWM_DBSIG_2 (0xB54BFB65)
    DWORD dwFlags;
    DWORD dwVersion;         ///< File version, currently 0x00030004

    BYTE aMasterSeed[16];    ///< Seed that gets hashed with the user key to form the final key
    UINT8 aEncryptionIV[16]; ///< IV used for content encryption

    DWORD dwGroups;          ///< Number of groups in the database
    DWORD dwEntries;         ///< Number of entries in the database

    BYTE aContentsHash[32];  ///< SHA-256 hash of the database, used for integrity checking

    BYTE aMasterSeed2[32];   ///< Seed used for the dwKeyEncRounds AES transformations
    DWORD dwKeyEncRounds;    ///< Number of master key transformations (default: 600,000)
} PW_DBHEADER;

/// Group structure, containing information about one group
typedef struct _PW_GROUP
{
    DWORD uGroupId;          ///< ID of the group. Unique identifier in one database.
    DWORD uImageId;          ///< Index of the icon in the image list to use for this group.
    char* pszGroupName;      ///< Name of the group (UTF-8, null-terminated).

    PW_TIME tCreation;       ///< Time when the group was created.
    PW_TIME tLastMod;        ///< Time when the group was last modified.
    PW_TIME tLastAccess;     ///< Time when the group was last accessed.
    PW_TIME tExpire;         ///< Time when the group will expire.

    USHORT usLevel;          ///< Indentation/depth level in the group tree.

    DWORD dwFlags;           ///< Used by KeePass internally (set to 0 for new structure).
} PW_GROUP;

/// Entry structure, containing information about one entry
typedef struct _PW_ENTRY
{
    BYTE uuid[16];           ///< Unique GUID identifying this entry (globally unique).
    DWORD uGroupId;          ///< ID of the group that contains this entry.
    DWORD uImageId;          ///< Index of the icon in the image list to use for this entry.

    char* pszTitle;          ///< Title (UTF-8, null-terminated).
    char* pszURL;            ///< URL (UTF-8, null-terminated).
    char* pszUserName;       ///< User name (UTF-8, null-terminated).

    DWORD uPasswordLen;      ///< Length of the password (required for memory protection).
    char* pszPassword;       ///< Password (UTF-8, may be encrypted, use unlock/lock functions).

    char* pszAdditional;     ///< Notes (UTF-8, null-terminated).

    PW_TIME tCreation;       ///< Time when the entry was created.
    PW_TIME tLastMod;        ///< Time when the entry was last modified.
    PW_TIME tLastAccess;     ///< Time when the entry was last accessed.
    PW_TIME tExpire;         ///< Time when the entry will expire.

    char* pszBinaryDesc;     ///< Description of the binary data contents.
    BYTE* pBinaryData;       ///< Attachment data (of length uBinaryDataLen), may be NULL.
    DWORD uBinaryDataLen;    ///< Length of the attachment data in bytes.
} PW_ENTRY;

/// Structure wrapping one UUID (GUID)
typedef struct _PW_UUID_STRUCT
{
    BYTE uuid[16]; ///< A GUID (globally unique identifier).
} PW_UUID_STRUCT;

/// Structure containing information about a database repair process
typedef struct _PWDB_REPAIR_INFO
{
    DWORD dwOriginalGroupCount;
    DWORD dwOriginalEntryCount;
    DWORD dwRecognizedMetaStreamCount;
} PWDB_REPAIR_INFO;

/// Structure containing information about one main menu item provided by a plugin
typedef struct
{
    DWORD dwFlags;           ///< Flags (enabled state, checkbox, popup, etc).
    DWORD dwState;           ///< State (checkbox checked, etc).
    DWORD dwIcon;
    char* lpCommandString;   ///< The menu item's text (UTF-8).
    DWORD dwCommandID;       ///< Set by KeePass, don't modify yourself.
    quint64 dwReserved;      ///< Reserved for future use, must be 0.
} KP_MENU_ITEM;

/// Structure used for entry validations by plugins
typedef struct
{
    const void* pOriginalEntry; ///< Pointer to the original PW_ENTRY.

    BYTE uuid[16];           ///< Unique GUID identifying this entry.
    DWORD uGroupIndex;       ///< Index of the group that contains this entry.
    DWORD uImageId;          ///< Index of the icon.

    const char* lpTitle;     ///< Title (UTF-8).
    const char* lpURL;       ///< URL (UTF-8).
    const char* lpUserName;  ///< User name (UTF-8).
    const char* lpPassword;  ///< Password (UTF-8, unencrypted).
    const char* lpAdditional;///< Notes (UTF-8).

    quint64 dwReserved;      ///< Reserved for future use, must be 0.
} KP_ENTRY;

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////
// Key provider structures

#pragma pack(push, 1)

/// Information about a key provider
typedef struct _KP_KEYPROV_INFO
{
    DWORD dwFlags;           ///< Reserved for future use, must be 0.
    const char* lpName;      ///< Unique display name of the key provider (UTF-8).
    DWORD dwImageIndex;      ///< Index of the icon shown in the combo box.
    quint64 dwReserved;      ///< Reserved for future use, must be 0.
} KP_KEYPROV_INFO;

/// A key returned by a key provider
typedef struct _KP_KEYPROV_KEY
{
    DWORD dwType;            ///< Type flags.
    DWORD dwFormat;          ///< Reserved for future use, must be 0.
    void* lpData;            ///< Key data pointer.
    DWORD dwDataSize;        ///< Size of the key (lpData) in bytes.
    quint64 dwReserved;      ///< Reserved for future use, must be 0.
} KP_KEYPROV_KEY;

/// Information structure used when querying keys from key providers
typedef struct _KP_KEYPROV_CONTEXT
{
    DWORD dwSize;            ///< Size of the KP_KEYPROV_CONTEXT structure.
    const char* lpProviderName; ///< Name of the provider that should generate the key.
    BOOL bCreatingNewKey;    ///< Specifies whether a new key is being generated.
    BOOL bConfirming;        ///< Specifies whether KeePass asks the user to confirm the key.
    BOOL bChanging;
    const char* lpDescriptiveName; ///< File name or some other descriptive string.
} KP_KEYPROV_CONTEXT;

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////
// Qt-Specific Helper Structures (not part of KDB format)

/// Meta stream structure for storing custom data in KDB files
struct PwMetaStream
{
    QString strName;
    QByteArray vData;
};

/// Custom key-value pair for plugin data
struct CustomKvp
{
    QString key;
    QString value;
};

/// Simple UI state stored in meta stream
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

/////////////////////////////////////////////////////////////////////////////
// Validation macro for debug builds

#ifdef QT_DEBUG
#define ASSERT_ENTRY(pp) \
    Q_ASSERT((pp) != nullptr); \
    Q_ASSERT((pp)->pszTitle != nullptr); \
    Q_ASSERT((pp)->pszUserName != nullptr); \
    Q_ASSERT((pp)->pszURL != nullptr); \
    Q_ASSERT((pp)->pszPassword != nullptr); \
    Q_ASSERT((pp)->pszAdditional != nullptr); \
    Q_ASSERT((pp)->pszBinaryDesc != nullptr); \
    if(((pp)->uBinaryDataLen != 0) && ((pp)->pBinaryData == nullptr)) { Q_ASSERT(false); }
#else
#define ASSERT_ENTRY(pp)
#endif

#endif // PW_STRUCTS_H
