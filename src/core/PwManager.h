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
#define PWM_PRODUCT_NAME       "KeePass Password Safe"
#define PWM_PRODUCT_NAME_SHORT "KeePass"
#define PWM_VERSION_STR        "1.43"
#define PWM_VERSION_DW         0x012B0000
#define PWM_VERSION_QW         0x0001002B00000000ULL

// Database file signature bytes (MUST match MFC version exactly)
#define PWM_DBSIG_1      0x9AA2D903
#define PWM_DBSIG_2      0xB54BFB65
#define PWM_DBVER_DW     0x00030004

// KeePass 2.x database signatures (for detection only)
#define PWM_DBSIG_1_KDBX_P 0x9AA2D903
#define PWM_DBSIG_2_KDBX_P 0xB54BFB66
#define PWM_DBSIG_1_KDBX_R 0x9AA2D903
#define PWM_DBSIG_2_KDBX_R 0xB54BFB67

// Standard constants
#define PWM_SESSION_KEY_SIZE     32
#define PWM_STD_KEYENCROUNDS     600000  // Default key transformation rounds

// Encryption algorithms
#define ALGO_AES         0
#define ALGO_TWOFISH     1

// Error codes
#define PWE_UNKNOWN                 0
#define PWE_SUCCESS                 1
#define PWE_INVALID_PARAM           2
#define PWE_NO_MEM                  3
#define PWE_INVALID_KEY             4
#define PWE_NOFILEACCESS_READ       5
#define PWE_NOFILEACCESS_WRITE      6
#define PWE_FILEERROR_READ          7
#define PWE_FILEERROR_WRITE         8
#define PWE_INVALID_RANDOMSOURCE    9
#define PWE_INVALID_FILESTRUCTURE  10
#define PWE_CRYPT_ERROR            11
#define PWE_INVALID_FILESIZE       12
#define PWE_INVALID_FILESIGNATURE  13
#define PWE_INVALID_FILEHEADER     14
#define PWE_NOFILEACCESS_READ_KEY  15
#define PWE_KEYPROV_INVALID_KEY    16
#define PWE_FILEERROR_VERIFY       17
#define PWE_UNSUPPORTED_KDBX       18
#define PWE_GETLASTERROR           19
#define PWE_DB_EMPTY               20
#define PWE_ATTACH_TOOLARGE        21

// Field flags (for Find function)
#define PWMF_TITLE              1
#define PWMF_USER               2
#define PWMF_URL                4
#define PWMF_PASSWORD           8
#define PWMF_ADDITIONAL        16
#define PWMF_GROUPNAME         32
#define PWMF_CREATION          64
#define PWMF_LASTMOD          128
#define PWMF_LASTACCESS       256
#define PWMF_EXPIRE           512
#define PWMF_UUID            1024

// Search flags
#define PWMS_REGEX       0x10000000

// Group flags
#define PWGF_EXPANDED    1

/// Password Manager - Core database management class
/// This class manages the KeePass database (KDB v1.x format)
/// Ported from MFC CPwManager to Qt
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
    DWORD getNumberOfEntries() const;
    DWORD getNumberOfGroups() const;
    DWORD getNumberOfItemsInGroup(const QString& groupName) const;
    DWORD getNumberOfItemsInGroupN(DWORD idGroup) const;

    // Entry access
    PW_ENTRY* getEntry(DWORD dwIndex);
    PW_ENTRY* getEntryByGroup(DWORD idGroup, DWORD dwIndex);
    DWORD getEntryByGroupN(DWORD idGroup, DWORD dwIndex) const;
    PW_ENTRY* getEntryByUuid(const BYTE* pUuid);
    DWORD getEntryByUuidN(const BYTE* pUuid) const;
    DWORD getEntryPosInGroup(const PW_ENTRY* pEntry) const;
    PW_ENTRY* getLastEditedEntry();

    // Group access
    PW_GROUP* getGroup(DWORD dwIndex);
    PW_GROUP* getGroupById(DWORD idGroup);
    DWORD getGroupByIdN(DWORD idGroup) const;
    DWORD getGroupId(const QString& groupName) const;
    DWORD getGroupIdByIndex(DWORD uGroupIndex) const;
    DWORD getLastChildGroup(DWORD dwParentGroupIndex) const;
    bool getGroupTree(DWORD idGroup, DWORD* pGroupIndexes) const;

    // Add/modify/delete
    bool addGroup(const PW_GROUP* pTemplate);
    bool addEntry(const PW_ENTRY* pTemplate);
    bool backupEntry(const PW_ENTRY* pe, bool* pbGroupCreated = nullptr);
    bool deleteEntry(DWORD dwIndex);
    bool deleteGroupById(DWORD uGroupId, bool bCreateBackupEntries);
    bool setGroup(DWORD dwIndex, const PW_GROUP* pTemplate);
    bool setEntry(DWORD dwIndex, const PW_ENTRY* pTemplate);

    // Password encryption/decryption in memory
    void lockEntryPassword(PW_ENTRY* pEntry);
    void unlockEntryPassword(PW_ENTRY* pEntry);

    // Database operations
    void newDatabase();
    int openDatabase(const QString& filePath, PWDB_REPAIR_INFO* pRepair = nullptr);
    int saveDatabase(const QString& filePath, BYTE* pWrittenDataHash32 = nullptr);

    // Move operations
    void moveEntry(DWORD idGroup, DWORD dwFrom, DWORD dwTo);
    bool moveGroup(DWORD dwFrom, DWORD dwTo);
    bool moveGroupEx(DWORD dwFromId, DWORD dwToId);
    bool moveGroupExDir(DWORD dwGroupId, int iDirection);

    // Sort operations
    void sortGroup(DWORD idGroup, DWORD dwSortByField);
    void sortGroupList();

    // Merge databases
    void mergeIn(PwManager* pDataSource, bool bCreateNewUUIDs, bool bCompareTimes);

    // Find operations
    DWORD find(const QString& findString, bool bCaseSensitive, DWORD searchFlags,
               DWORD nStart, DWORD nEndExcl, QString* pError = nullptr);
    DWORD findEx(const QString& findString, bool bCaseSensitive, DWORD searchFlags,
                 DWORD nStart, QString* pError = nullptr);

    // Encryption settings
    int getAlgorithm() const;
    bool setAlgorithm(int nAlgorithm);
    DWORD getKeyEncRounds() const;
    void setKeyEncRounds(DWORD dwRounds);

    // Group tree management
    void fixGroupTree();
    void substEntryGroupIds(DWORD dwExistingId, DWORD dwNewId);

    // Database header and keys
    const PW_DBHEADER* getLastDatabaseHeader() const;
    void getRawMasterKey(BYTE* pStorage);
    void setRawMasterKey(const BYTE* pNewKey);
    void clearMasterKey(bool bClearKey, bool bClearTransformedKey);
    QString getKeySource() const;

    // Properties and custom data
    QString getPropertyString(DWORD dwPropertyId) const;
    bool setPropertyString(DWORD dwPropertyId, const QString& value);
    QVector<QString>* accessPropertyStrArray(DWORD dwPropertyId);
    bool setCustomKvp(const QString& key, const QString& value);
    QString getCustomKvp(const QString& key) const;

    // Settings
    void setTransactedFileWrites(bool bTransacted) { m_bUseTransactedFileWrites = bTransacted; }
    QColor getColor() const;
    void setColor(const QColor& clr);

    // UI state (public for direct access like MFC version)
    DWORD m_dwLastSelectedGroupId;
    DWORD m_dwLastTopVisibleGroupId;
    BYTE m_aLastSelectedEntryUuid[16];
    BYTE m_aLastTopVisibleEntryUuid[16];

private:
    void cleanUp();
    void detMetaInfo();

    void allocEntries(DWORD uEntries);
    void deleteEntryList(bool bFreeStrings);
    void allocGroups(DWORD uGroups);
    void deleteGroupList(bool bFreeStrings);

    bool readGroupField(USHORT usFieldType, DWORD dwFieldSize,
                       const BYTE* pData, PW_GROUP* pGroup, PWDB_REPAIR_INFO* pRepair);
    bool readEntryField(USHORT usFieldType, DWORD dwFieldSize,
                       const BYTE* pData, PW_ENTRY* pEntry, PWDB_REPAIR_INFO* pRepair);
    bool readExtData(const BYTE* pData, DWORD dwDataSize, PW_GROUP* pg,
                    PW_ENTRY* pe, PWDB_REPAIR_INFO* pRepair);
    void writeExtData(QByteArray& data);
    static void writeExtDataField(QByteArray& data, USHORT usFieldType,
                                  const BYTE* pData, DWORD dwFieldSize);

    bool addAllMetaStreams();
    DWORD loadAndRemoveAllMetaStreams(bool bAcceptUnknown);
    bool addMetaStream(const QString& metaDataDesc, BYTE* pData, DWORD dwLength);
    bool isMetaStream(const PW_ENTRY* p) const;
    void parseMetaStream(PW_ENTRY* p, bool bAcceptUnknown);
    bool canIgnoreUnknownMetaStream(const PwMetaStream& msUnknown) const;

    bool transformMasterKey(const BYTE* pKeySeed);
    static void hashHeaderWithoutContentHash(const BYTE* pbHeader, QByteArray& vHash);

    DWORD deleteLostEntries();
    void moveInternal(DWORD dwFrom, DWORD dwTo);

    static QByteArray serializeCustomKvp(const CustomKvp& kvp);
    static bool deserializeCustomKvp(const BYTE* pStream, CustomKvp& kvpBuffer);

    void protectMasterKey(bool bProtectKey);
    void protectTransformedMasterKey(bool bProtectKey);

    // Member variables (same as MFC version for compatibility)
    PW_ENTRY* m_pEntries;
    DWORD m_dwMaxEntries;
    DWORD m_dwNumEntries;

    PW_GROUP* m_pGroups;
    DWORD m_dwMaxGroups;
    DWORD m_dwNumGroups;

    PW_DBHEADER m_dbLastHeader;
    PW_ENTRY* m_pLastEditedEntry;
    QByteArray m_vHeaderHash;

    BYTE m_pSessionKey[PWM_SESSION_KEY_SIZE];
    BYTE m_pMasterKey[32];
    BYTE m_pTransformedMasterKey[32];
    int m_nAlgorithm;
    DWORD m_dwKeyEncRounds;
    QString m_strKeySource;

    QString m_strDefaultUserName;
    QVector<QString> m_vSearchHistory;
    QVector<CustomKvp> m_vCustomKVPs;
    QVector<PwMetaStream> m_vUnknownMetaStreams;

    bool m_bUseTransactedFileWrites;
    QColor m_clr;
};

#endif // PW_MANAGER_H
