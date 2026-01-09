/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef PW_MANAGER_H
#define PW_MANAGER_H

#include <QString>
#include <QVector>
#include <QColor>
#include "PwStructs.h"

// General product information
namespace PwProduct {
    constexpr const char* NAME = "KeePass Password Safe";
    constexpr const char* NAME_SHORT = "KeePass";
    constexpr const char* VERSION_STR = "1.43";
    constexpr quint32 VERSION_DW = 0x012B0000;
    constexpr quint64 VERSION_QW = 0x0001002B00000000ULL;
}

// Database file signature bytes (MUST match MFC version exactly)
namespace PwDbSignature {
    constexpr quint32 SIG1 = 0x9AA2D903;
    constexpr quint32 SIG2 = 0xB54BFB65;
    constexpr quint32 VERSION = 0x00030004;

    // KeePass 2.x database signatures (for detection only)
    constexpr quint32 SIG1_KDBX_PRE = 0x9AA2D903;
    constexpr quint32 SIG2_KDBX_PRE = 0xB54BFB66;
    constexpr quint32 SIG1_KDBX_REL = 0x9AA2D903;
    constexpr quint32 SIG2_KDBX_REL = 0xB54BFB67;
}

// Standard constants
namespace PwConstants {
    constexpr size_t SESSION_KEY_SIZE = 32;
    constexpr quint32 STD_KEYENC_ROUNDS = 600000;  ///< Default key transformation rounds
}

// Encryption algorithms
enum class PwAlgorithm : quint8 {
    AES = 0,
    TWOFISH = 1
};

// Error codes
enum class PwError : quint8 {
    UNKNOWN = 0,
    SUCCESS = 1,
    INVALID_PARAM = 2,
    NO_MEM = 3,
    INVALID_KEY = 4,
    NOFILEACCESS_READ = 5,
    NOFILEACCESS_WRITE = 6,
    FILEERROR_READ = 7,
    FILEERROR_WRITE = 8,
    INVALID_RANDOMSOURCE = 9,
    INVALID_FILESTRUCTURE = 10,
    CRYPT_ERROR = 11,
    INVALID_FILESIZE = 12,
    INVALID_FILESIGNATURE = 13,
    INVALID_FILEHEADER = 14,
    NOFILEACCESS_READ_KEY = 15,
    KEYPROV_INVALID_KEY = 16,
    FILEERROR_VERIFY = 17,
    UNSUPPORTED_KDBX = 18,
    GETLASTERROR = 19,
    DB_EMPTY = 20,
    ATTACH_TOOLARGE = 21
};

// Field flags (for Find function)
namespace PwFieldFlags {
    constexpr quint32 TITLE = 1;
    constexpr quint32 USER = 2;
    constexpr quint32 URL = 4;
    constexpr quint32 PASSWORD = 8;
    constexpr quint32 ADDITIONAL = 16;
    constexpr quint32 GROUPNAME = 32;
    constexpr quint32 CREATION = 64;
    constexpr quint32 LASTMOD = 128;
    constexpr quint32 LASTACCESS = 256;
    constexpr quint32 EXPIRE = 512;
    constexpr quint32 UUID = 1024;
}

// Search flags
namespace PwSearchFlags {
    constexpr quint32 REGEX = 0x10000000;
}

// Group flags
namespace PwGroupFlags {
    constexpr quint32 EXPANDED = 1;
}

// Legacy compatibility macros (temporary - to be removed)
// TODO: Migrate all code to use new namespaced constants
#define PWM_PRODUCT_NAME       PwProduct::NAME
#define PWM_PRODUCT_NAME_SHORT PwProduct::NAME_SHORT
#define PWM_VERSION_STR        PwProduct::VERSION_STR
#define PWM_VERSION_DW         PwProduct::VERSION_DW
#define PWM_VERSION_QW         PwProduct::VERSION_QW
#define PWM_DBSIG_1            PwDbSignature::SIG1
#define PWM_DBSIG_2            PwDbSignature::SIG2
#define PWM_DBVER_DW           PwDbSignature::VERSION
#define PWM_DBSIG_1_KDBX_P     PwDbSignature::SIG1_KDBX_PRE
#define PWM_DBSIG_2_KDBX_P     PwDbSignature::SIG2_KDBX_PRE
#define PWM_DBSIG_1_KDBX_R     PwDbSignature::SIG1_KDBX_REL
#define PWM_DBSIG_2_KDBX_R     PwDbSignature::SIG2_KDBX_REL
#define PWM_SESSION_KEY_SIZE   PwConstants::SESSION_KEY_SIZE
#define PWM_STD_KEYENCROUNDS   PwConstants::STD_KEYENC_ROUNDS
#define ALGO_AES               static_cast<int>(PwAlgorithm::AES)
#define ALGO_TWOFISH           static_cast<int>(PwAlgorithm::TWOFISH)
#define PWE_UNKNOWN            static_cast<int>(PwError::UNKNOWN)
#define PWE_SUCCESS            static_cast<int>(PwError::SUCCESS)
#define PWE_INVALID_PARAM      static_cast<int>(PwError::INVALID_PARAM)
#define PWE_NO_MEM             static_cast<int>(PwError::NO_MEM)
#define PWE_INVALID_KEY        static_cast<int>(PwError::INVALID_KEY)
#define PWE_NOFILEACCESS_READ  static_cast<int>(PwError::NOFILEACCESS_READ)
#define PWE_NOFILEACCESS_WRITE static_cast<int>(PwError::NOFILEACCESS_WRITE)
#define PWE_FILEERROR_READ     static_cast<int>(PwError::FILEERROR_READ)
#define PWE_FILEERROR_WRITE    static_cast<int>(PwError::FILEERROR_WRITE)
#define PWE_INVALID_RANDOMSOURCE static_cast<int>(PwError::INVALID_RANDOMSOURCE)
#define PWE_INVALID_FILESTRUCTURE static_cast<int>(PwError::INVALID_FILESTRUCTURE)
#define PWE_CRYPT_ERROR        static_cast<int>(PwError::CRYPT_ERROR)
#define PWE_INVALID_FILESIZE   static_cast<int>(PwError::INVALID_FILESIZE)
#define PWE_INVALID_FILESIGNATURE static_cast<int>(PwError::INVALID_FILESIGNATURE)
#define PWE_INVALID_FILEHEADER static_cast<int>(PwError::INVALID_FILEHEADER)
#define PWE_NOFILEACCESS_READ_KEY static_cast<int>(PwError::NOFILEACCESS_READ_KEY)
#define PWE_KEYPROV_INVALID_KEY static_cast<int>(PwError::KEYPROV_INVALID_KEY)
#define PWE_FILEERROR_VERIFY   static_cast<int>(PwError::FILEERROR_VERIFY)
#define PWE_UNSUPPORTED_KDBX   static_cast<int>(PwError::UNSUPPORTED_KDBX)
#define PWE_GETLASTERROR       static_cast<int>(PwError::GETLASTERROR)
#define PWE_DB_EMPTY           static_cast<int>(PwError::DB_EMPTY)
#define PWE_ATTACH_TOOLARGE    static_cast<int>(PwError::ATTACH_TOOLARGE)
#define PWMF_TITLE             PwFieldFlags::TITLE
#define PWMF_USER              PwFieldFlags::USER
#define PWMF_URL               PwFieldFlags::URL
#define PWMF_PASSWORD          PwFieldFlags::PASSWORD
#define PWMF_ADDITIONAL        PwFieldFlags::ADDITIONAL
#define PWMF_GROUPNAME         PwFieldFlags::GROUPNAME
#define PWMF_CREATION          PwFieldFlags::CREATION
#define PWMF_LASTMOD           PwFieldFlags::LASTMOD
#define PWMF_LASTACCESS        PwFieldFlags::LASTACCESS
#define PWMF_EXPIRE            PwFieldFlags::EXPIRE
#define PWMF_UUID              PwFieldFlags::UUID
#define PWMS_REGEX             PwSearchFlags::REGEX
#define PWGF_EXPANDED          PwGroupFlags::EXPANDED

/**
 * @class PwManager
 * @brief Core database management class for KeePass KDB v1.x format
 *
 * This class handles all database operations including:
 * - Opening and saving KDB v1.x files
 * - Managing groups and entries
 * - Master key handling and encryption
 * - In-memory password protection
 * - Database merging and repair
 *
 * Ported from MFC CPwManager to Qt.
 *
 * @note Migration Status:
 * - Data types: Migrated to Qt types (quint8, quint16, quint32, QString)
 * - Member variables: Currently use MFC-style Hungarian notation (m_pEntries, m_dwNumEntries)
 *   TODO: Future refactoring pass will rename to KDE style (m_entries, m_numEntries)
 * - Constants: Migrated to constexpr with legacy #define compatibility layer
 * - File format: 100% binary compatible with MFC KeePass v1.43
 *
 * @warning CRITICAL: Any changes to data structures or serialization logic must
 *          maintain byte-level compatibility with the KDB v1.x file format.
 */
class PwManager
{
    Q_DISABLE_COPY(PwManager)

public:
    PwManager();
    virtual ~PwManager();

    void initPrimaryInstance();

    // Static utility methods
    static void getNeverExpireTime(PW_TIME* pPwTime);

    // Master key management
    int setMasterKey(const QString& masterKey, bool bDiskDrive, const QString& secondKey,
                     bool bOverwrite, const QString& providerName);

    // Database info
    [[nodiscard]] quint32 getNumberOfEntries() const;
    [[nodiscard]] quint32 getNumberOfGroups() const;
    [[nodiscard]] quint32 getNumberOfItemsInGroup(const QString& groupName) const;
    [[nodiscard]] quint32 getNumberOfItemsInGroupN(quint32 idGroup) const;

    // Entry access
    PW_ENTRY* getEntry(quint32 dwIndex);
    PW_ENTRY* getEntryByGroup(quint32 idGroup, quint32 dwIndex);
    [[nodiscard]] quint32 getEntryByGroupN(quint32 idGroup, quint32 dwIndex) const;
    PW_ENTRY* getEntryByUuid(const quint8* pUuid);
    [[nodiscard]] quint32 getEntryByUuidN(const quint8* pUuid) const;
    [[nodiscard]] quint32 getEntryPosInGroup(const PW_ENTRY* pEntry) const;
    PW_ENTRY* getLastEditedEntry();

    // Group access
    PW_GROUP* getGroup(quint32 dwIndex);
    PW_GROUP* getGroupById(quint32 idGroup);
    [[nodiscard]] quint32 getGroupByIdN(quint32 idGroup) const;
    [[nodiscard]] quint32 getGroupId(const QString& groupName) const;
    [[nodiscard]] quint32 getGroupIdByIndex(quint32 uGroupIndex) const;
    [[nodiscard]] quint32 getLastChildGroup(quint32 dwParentGroupIndex) const;
    bool getGroupTree(quint32 idGroup, quint32* pGroupIndexes) const;

    // Add/modify/delete
    bool addGroup(const PW_GROUP* pTemplate);
    bool addEntry(const PW_ENTRY* pTemplate);
    bool backupEntry(const PW_ENTRY* pe, bool* pbGroupCreated = nullptr);
    bool deleteEntry(quint32 dwIndex);
    bool deleteGroupById(quint32 uGroupId, bool bCreateBackupEntries);
    bool setGroup(quint32 dwIndex, const PW_GROUP* pTemplate);
    bool setEntry(quint32 dwIndex, const PW_ENTRY* pTemplate);

    // Password encryption/decryption in memory
    void lockEntryPassword(PW_ENTRY* pEntry);
    void unlockEntryPassword(PW_ENTRY* pEntry);

    // Database operations
    void newDatabase();
    int openDatabase(const QString& filePath, PWDB_REPAIR_INFO* pRepair = nullptr);
    int saveDatabase(const QString& filePath, quint8* pWrittenDataHash32 = nullptr);

    // Move operations
    void moveEntry(quint32 idGroup, quint32 dwFrom, quint32 dwTo);
    bool moveGroup(quint32 dwFrom, quint32 dwTo);
    bool moveGroupEx(quint32 dwFromId, quint32 dwToId);
    bool moveGroupExDir(quint32 dwGroupId, int iDirection);

    // Sort operations
    void sortGroup(quint32 idGroup, quint32 dwSortByField);
    void sortGroupList();

    // Merge databases
    void mergeIn(PwManager* pDataSource, bool bCreateNewUUIDs, bool bCompareTimes);

    // Find operations
    quint32 find(const QString& findString, bool bCaseSensitive, quint32 searchFlags,
                 quint32 nStart, quint32 nEndExcl, QString* pError = nullptr);
    quint32 findEx(const QString& findString, bool bCaseSensitive, quint32 searchFlags,
                   quint32 nStart, QString* pError = nullptr);
    QList<quint32> findAll(const QString& findString, bool bCaseSensitive, quint32 searchFlags,
                           bool excludeBackups, bool excludeExpired, QString* pError = nullptr);

    // Expiration-based searches
    QList<quint32> findExpiredEntries(bool excludeBackups = true, bool excludeTANs = true);
    QList<quint32> findSoonToExpireEntries(int days = 7, bool excludeBackups = true, bool excludeTANs = true);

    // Encryption settings
    [[nodiscard]] int getAlgorithm() const;
    bool setAlgorithm(int nAlgorithm);
    [[nodiscard]] quint32 getKeyEncRounds() const;
    void setKeyEncRounds(quint32 dwRounds);

    // Group tree management
    void fixGroupTree();
    void substEntryGroupIds(quint32 dwExistingId, quint32 dwNewId);

    // Database header and keys
    [[nodiscard]] const PW_DBHEADER* getLastDatabaseHeader() const;
    void getRawMasterKey(quint8* pStorage);
    void setRawMasterKey(const quint8* pNewKey);
    void clearMasterKey(bool bClearKey, bool bClearTransformedKey);
    [[nodiscard]] QString getKeySource() const;

    // Properties and custom data
    [[nodiscard]] QString getPropertyString(quint32 dwPropertyId) const;
    bool setPropertyString(quint32 dwPropertyId, const QString& value);
    QVector<QString>* accessPropertyStrArray(quint32 dwPropertyId);
    bool setCustomKvp(const QString& key, const QString& value);
    [[nodiscard]] QString getCustomKvp(const QString& key) const;

    // Settings
    void setTransactedFileWrites(bool bTransacted) { m_bUseTransactedFileWrites = bTransacted; }
    [[nodiscard]] QColor getColor() const;
    void setColor(const QColor& clr);
    [[nodiscard]] QString getDefaultUserName() const;
    void setDefaultUserName(const QString& strUserName);

    // UI state (public for direct access like MFC version)
    quint32 m_dwLastSelectedGroupId;
    quint32 m_dwLastTopVisibleGroupId;
    // NOLINTBEGIN(modernize-avoid-c-arrays) - Public API compatibility with MFC version
    quint8 m_aLastSelectedEntryUuid[16];
    quint8 m_aLastTopVisibleEntryUuid[16];
    // NOLINTEND(modernize-avoid-c-arrays)

private:
    void cleanUp();
    void detMetaInfo();

    void allocEntries(quint32 uEntries);
    void deleteEntryList(bool bFreeStrings);
    void allocGroups(quint32 uGroups);
    void deleteGroupList(bool bFreeStrings);

    bool readGroupField(quint16 usFieldType, quint32 dwFieldSize,
                       const quint8* pData, PW_GROUP* pGroup, PWDB_REPAIR_INFO* pRepair);
    bool readEntryField(quint16 usFieldType, quint32 dwFieldSize,
                       const quint8* pData, PW_ENTRY* pEntry, PWDB_REPAIR_INFO* pRepair);
    bool readExtData(const quint8* pData, quint32 dwDataSize, PW_GROUP* pg,
                    PW_ENTRY* pe, PWDB_REPAIR_INFO* pRepair);
    void writeExtData(QByteArray& data);
    static void writeExtDataField(QByteArray& data, quint16 usFieldType,
                                  const quint8* pData, quint32 dwFieldSize);

    bool addAllMetaStreams();
    quint32 loadAndRemoveAllMetaStreams(bool bAcceptUnknown);
    bool addMetaStream(const QString& metaDataDesc, quint8* pData, quint32 dwLength);
    bool isMetaStream(const PW_ENTRY* p) const;
    void parseMetaStream(PW_ENTRY* p, bool bAcceptUnknown);
    bool canIgnoreUnknownMetaStream(const PwMetaStream& msUnknown) const;

    bool transformMasterKey(const quint8* pKeySeed);
    static void hashHeaderWithoutContentHash(const quint8* pbHeader, QByteArray& vHash);

    quint32 deleteLostEntries();
    void moveInternal(quint32 dwFrom, quint32 dwTo);

    static QByteArray serializeCustomKvp(const CustomKvp& kvp);
    static bool deserializeCustomKvp(const quint8* pStream, CustomKvp& kvpBuffer);

    void protectMasterKey(bool bProtectKey);
    void protectTransformedMasterKey(bool bProtectKey);

    // Member variables - Qt types and naming conventions
    PW_ENTRY* m_pEntries;      // Pointer kept as-is (array of entries)
    quint32 m_maxEntries;      // Maximum allocated entries
    quint32 m_numEntries;      // Current number of entries

    PW_GROUP* m_pGroups;       // Pointer kept as-is (array of groups)
    quint32 m_maxGroups;       // Maximum allocated groups
    quint32 m_numGroups;       // Current number of groups

    PW_DBHEADER m_dbLastHeader;
    PW_ENTRY* m_pLastEditedEntry;
    QByteArray m_vHeaderHash;

    // NOLINTBEGIN(modernize-avoid-c-arrays) - Cryptographic keys interface with C APIs (OpenSSL)
    quint8 m_sessionKey[PWM_SESSION_KEY_SIZE];  // Session key for in-memory encryption
    quint8 m_masterKey[32];                      // Hashed master password
    quint8 m_transformedMasterKey[32];           // Transformed key after N rounds
    // NOLINTEND(modernize-avoid-c-arrays)
    int m_nAlgorithm;                            // Encryption algorithm (ALGO_AES or ALGO_TWOFISH)
    quint32 m_keyEncRounds;                      // Number of key transformation rounds
    QString m_strKeySource;

    QString m_strDefaultUserName;
    QVector<QString> m_vSearchHistory;
    QVector<CustomKvp> m_vCustomKVPs;
    QVector<PwMetaStream> m_vUnknownMetaStreams;

    bool m_bUseTransactedFileWrites;
    QColor m_clr;
};

#endif // PW_MANAGER_H
