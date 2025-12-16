/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "PwManager.h"
#include "crypto/KeyTransform.h"
#include "crypto/MemoryProtection.h"
#include "crypto/SHA256.h"
#include "util/Random.h"
#include "util/MemUtil.h"
#include <QFile>
#include <QDateTime>
#include <cstring>
#include <cstdlib>

// Initial allocation sizes
#define PWM_NUM_INITIAL_ENTRIES 256
#define PWM_NUM_INITIAL_GROUPS  32

PwManager::PwManager()
    : m_pEntries(nullptr)
    , m_dwMaxEntries(0)
    , m_dwNumEntries(0)
    , m_pGroups(nullptr)
    , m_dwMaxGroups(0)
    , m_dwNumGroups(0)
    , m_pLastEditedEntry(nullptr)
    , m_nAlgorithm(ALGO_AES)
    , m_dwKeyEncRounds(PWM_STD_KEYENCROUNDS)
    , m_bUseTransactedFileWrites(true)
    , m_clr(Qt::white)
{
    // Initialize header
    std::memset(&m_dbLastHeader, 0, sizeof(PW_DBHEADER));
    m_dbLastHeader.dwSignature1 = PWM_DBSIG_1;
    m_dbLastHeader.dwSignature2 = PWM_DBSIG_2;
    m_dbLastHeader.dwVersion = PWM_DBVER_DW;
    m_dbLastHeader.dwKeyEncRounds = PWM_STD_KEYENCROUNDS;

    // Initialize keys to zero
    std::memset(m_pSessionKey, 0, PWM_SESSION_KEY_SIZE);
    std::memset(m_pMasterKey, 0, 32);
    std::memset(m_pTransformedMasterKey, 0, 32);

    // Initialize UUID arrays
    std::memset(m_aLastSelectedEntryUuid, 0, 16);
    std::memset(m_aLastTopVisibleEntryUuid, 0, 16);

    m_dwLastSelectedGroupId = 0;
    m_dwLastTopVisibleGroupId = 0;

    // Generate session key for in-memory password encryption
    Random::fillBuffer(m_pSessionKey, PWM_SESSION_KEY_SIZE);
}

PwManager::~PwManager()
{
    cleanUp();

    // Securely erase sensitive data
    MemUtil::mem_erase(m_pSessionKey, PWM_SESSION_KEY_SIZE);
    MemUtil::mem_erase(m_pMasterKey, 32);
    MemUtil::mem_erase(m_pTransformedMasterKey, 32);
}

void PwManager::initPrimaryInstance()
{
    // Initialize for first use
    // In MFC version, this sets up some global state
    // For Qt version, we may not need this
}

void PwManager::cleanUp()
{
    // Delete all entries and groups
    deleteEntryList(true);
    deleteGroupList(true);

    m_pLastEditedEntry = nullptr;

    // Clear metadata
    m_vSearchHistory.clear();
    m_vCustomKVPs.clear();
    m_vUnknownMetaStreams.clear();
    m_strDefaultUserName.clear();
    m_strKeySource.clear();
    m_vHeaderHash.clear();
}

void PwManager::allocEntries(DWORD uEntries)
{
    Q_ASSERT(m_pEntries == nullptr);
    Q_ASSERT(uEntries > 0);

    m_pEntries = new PW_ENTRY[uEntries];
    std::memset(m_pEntries, 0, uEntries * sizeof(PW_ENTRY));

    m_dwMaxEntries = uEntries;
    m_dwNumEntries = 0;
}

void PwManager::deleteEntryList(bool bFreeStrings)
{
    if (m_pEntries == nullptr)
        return;

    if (bFreeStrings) {
        for (DWORD i = 0; i < m_dwNumEntries; ++i) {
            PW_ENTRY* e = &m_pEntries[i];

            // Unlock password before freeing (if it was locked)
            if (e->pszPassword && e->uPasswordLen > 0) {
                unlockEntryPassword(e);
            }

            // Free strings
            delete[] e->pszTitle;
            delete[] e->pszURL;
            delete[] e->pszUserName;
            delete[] e->pszPassword;
            delete[] e->pszAdditional;
            delete[] e->pszBinaryDesc;
            delete[] e->pBinaryData;

            // Zero out the structure
            std::memset(e, 0, sizeof(PW_ENTRY));
        }
    }

    delete[] m_pEntries;
    m_pEntries = nullptr;
    m_dwMaxEntries = 0;
    m_dwNumEntries = 0;
}

void PwManager::allocGroups(DWORD uGroups)
{
    Q_ASSERT(m_pGroups == nullptr);
    Q_ASSERT(uGroups > 0);

    m_pGroups = new PW_GROUP[uGroups];
    std::memset(m_pGroups, 0, uGroups * sizeof(PW_GROUP));

    m_dwMaxGroups = uGroups;
    m_dwNumGroups = 0;
}

void PwManager::deleteGroupList(bool bFreeStrings)
{
    if (m_pGroups == nullptr)
        return;

    if (bFreeStrings) {
        for (DWORD i = 0; i < m_dwNumGroups; ++i) {
            PW_GROUP* g = &m_pGroups[i];

            // Free strings
            delete[] g->pszGroupName;

            // Zero out the structure
            std::memset(g, 0, sizeof(PW_GROUP));
        }
    }

    delete[] m_pGroups;
    m_pGroups = nullptr;
    m_dwMaxGroups = 0;
    m_dwNumGroups = 0;
}

DWORD PwManager::getNumberOfEntries() const
{
    return m_dwNumEntries;
}

DWORD PwManager::getNumberOfGroups() const
{
    return m_dwNumGroups;
}

PW_ENTRY* PwManager::getEntry(DWORD dwIndex)
{
    if (dwIndex >= m_dwNumEntries)
        return nullptr;
    return &m_pEntries[dwIndex];
}

PW_GROUP* PwManager::getGroup(DWORD dwIndex)
{
    if (dwIndex >= m_dwNumGroups)
        return nullptr;
    return &m_pGroups[dwIndex];
}

void PwManager::lockEntryPassword(PW_ENTRY* pEntry)
{
    if (!pEntry || !pEntry->pszPassword || pEntry->uPasswordLen == 0)
        return;

    // XOR password bytes with session key
    // This is a simple but effective in-memory encryption
    BYTE* password = reinterpret_cast<BYTE*>(pEntry->pszPassword);
    for (DWORD i = 0; i < pEntry->uPasswordLen; ++i) {
        password[i] ^= m_pSessionKey[i % PWM_SESSION_KEY_SIZE];
    }
}

void PwManager::unlockEntryPassword(PW_ENTRY* pEntry)
{
    if (!pEntry || !pEntry->pszPassword || pEntry->uPasswordLen == 0)
        return;

    // XOR again to restore original (XOR is reversible)
    BYTE* password = reinterpret_cast<BYTE*>(pEntry->pszPassword);
    for (DWORD i = 0; i < pEntry->uPasswordLen; ++i) {
        password[i] ^= m_pSessionKey[i % PWM_SESSION_KEY_SIZE];
    }
}

void PwManager::newDatabase()
{
    // Clean up existing data
    cleanUp();

    // Allocate initial storage
    allocEntries(PWM_NUM_INITIAL_ENTRIES);
    allocGroups(PWM_NUM_INITIAL_GROUPS);

    // Reset header
    std::memset(&m_dbLastHeader, 0, sizeof(PW_DBHEADER));
    m_dbLastHeader.dwSignature1 = PWM_DBSIG_1;
    m_dbLastHeader.dwSignature2 = PWM_DBSIG_2;
    m_dbLastHeader.dwVersion = PWM_DBVER_DW;
    m_dbLastHeader.dwKeyEncRounds = m_dwKeyEncRounds;

    // Generate new random seeds
    Random::fillBuffer(m_dbLastHeader.aMasterSeed, 16);
    Random::fillBuffer(m_dbLastHeader.aEncryptionIV, 16);
    Random::fillBuffer(m_dbLastHeader.aMasterSeed2, 32);

    m_dwNumEntries = 0;
    m_dwNumGroups = 0;
}

void PwManager::getNeverExpireTime(PW_TIME* pPwTime)
{
    if (!pPwTime)
        return;

    pPwTime->shYear = 2999;
    pPwTime->btMonth = 12;
    pPwTime->btDay = 28;
    pPwTime->btHour = 23;
    pPwTime->btMinute = 59;
    pPwTime->btSecond = 59;
}

int PwManager::setMasterKey(const QString& masterKey, bool bDiskDrive,
                            const QString& secondKey, bool bOverwrite,
                            const QString& providerName)
{
    // TODO: Implement full master key setup
    // This is complex and involves:
    // 1. Hashing the password
    // 2. Reading key file if specified
    // 3. XOR-ing password hash with key file hash
    // 4. Key provider support
    // For now, basic implementation:

    if (masterKey.isEmpty() && secondKey.isEmpty())
        return PWE_INVALID_KEY;

    QByteArray keyData;

    if (!masterKey.isEmpty()) {
        // Hash the password with SHA-256
        QByteArray passwordBytes = masterKey.toUtf8();
        keyData = SHA256::hash(passwordBytes);
    }

    if (!secondKey.isEmpty() && bDiskDrive) {
        // TODO: Read key file and XOR with password hash
        // For now, just note that key files are supported
        m_strKeySource = QString("Password + Key File");
    } else {
        m_strKeySource = QString("Password");
    }

    // Copy to master key
    if (keyData.size() >= 32) {
        std::memcpy(m_pMasterKey, keyData.constData(), 32);
    }

    return PWE_SUCCESS;
}

int PwManager::getAlgorithm() const
{
    return m_nAlgorithm;
}

bool PwManager::setAlgorithm(int nAlgorithm)
{
    if (nAlgorithm != ALGO_AES && nAlgorithm != ALGO_TWOFISH)
        return false;

    m_nAlgorithm = nAlgorithm;
    return true;
}

DWORD PwManager::getKeyEncRounds() const
{
    return m_dwKeyEncRounds;
}

void PwManager::setKeyEncRounds(DWORD dwRounds)
{
    m_dwKeyEncRounds = dwRounds;
}

const PW_DBHEADER* PwManager::getLastDatabaseHeader() const
{
    return &m_dbLastHeader;
}

QString PwManager::getKeySource() const
{
    return m_strKeySource;
}

QColor PwManager::getColor() const
{
    return m_clr;
}

void PwManager::setColor(const QColor& clr)
{
    m_clr = clr;
}

bool PwManager::transformMasterKey(const BYTE* pKeySeed)
{
    if (!pKeySeed)
        return false;

    // Copy master key to transformed key buffer
    std::memcpy(m_pTransformedMasterKey, m_pMasterKey, 32);

    // Perform key transformation using OpenSSL
    if (!KeyTransform::transform256(m_dwKeyEncRounds, m_pTransformedMasterKey, pKeySeed)) {
        MemUtil::mem_erase(m_pTransformedMasterKey, 32);
        return false;
    }

    // Hash the transformed key with SHA-256
    QByteArray transformedHash = SHA256::hash(
        QByteArray(reinterpret_cast<const char*>(m_pTransformedMasterKey), 32));

    std::memcpy(m_pTransformedMasterKey, transformedHash.constData(), 32);

    return true;
}

int PwManager::openDatabase(const QString& filePath, PWDB_REPAIR_INFO* pRepair)
{
    // TODO: Implement full database opening
    // This is the most critical function for KDB compatibility
    // Will be implemented in next iteration

    Q_UNUSED(filePath);
    Q_UNUSED(pRepair);

    return PWE_INVALID_FILESTRUCTURE; // Placeholder
}

int PwManager::saveDatabase(const QString& filePath, BYTE* pWrittenDataHash32)
{
    // TODO: Implement full database saving
    // This is the most critical function for KDB compatibility
    // Will be implemented in next iteration

    Q_UNUSED(filePath);
    Q_UNUSED(pWrittenDataHash32);

    return PWE_FILEERROR_WRITE; // Placeholder
}

// Stub implementations for remaining methods
// These will be implemented as needed

DWORD PwManager::getNumberOfItemsInGroup(const QString& groupName) const
{
    Q_UNUSED(groupName);
    return 0;
}

DWORD PwManager::getNumberOfItemsInGroupN(DWORD idGroup) const
{
    DWORD count = 0;
    for (DWORD i = 0; i < m_dwNumEntries; ++i) {
        if (m_pEntries[i].uGroupId == idGroup)
            count++;
    }
    return count;
}

PW_ENTRY* PwManager::getEntryByUuid(const BYTE* pUuid)
{
    if (!pUuid)
        return nullptr;

    for (DWORD i = 0; i < m_dwNumEntries; ++i) {
        if (std::memcmp(m_pEntries[i].uuid, pUuid, 16) == 0)
            return &m_pEntries[i];
    }

    return nullptr;
}

PW_GROUP* PwManager::getGroupById(DWORD idGroup)
{
    for (DWORD i = 0; i < m_dwNumGroups; ++i) {
        if (m_pGroups[i].uGroupId == idGroup)
            return &m_pGroups[i];
    }

    return nullptr;
}

DWORD PwManager::getGroupByIdN(DWORD idGroup) const
{
    for (DWORD i = 0; i < m_dwNumGroups; ++i) {
        if (m_pGroups[i].uGroupId == idGroup)
            return i;
    }

    return static_cast<DWORD>(-1);
}

PW_ENTRY* PwManager::getLastEditedEntry()
{
    return m_pLastEditedEntry;
}

bool PwManager::addGroup(const PW_GROUP* pTemplate)
{
    Q_UNUSED(pTemplate);
    // TODO: Implement
    return false;
}

bool PwManager::addEntry(const PW_ENTRY* pTemplate)
{
    Q_UNUSED(pTemplate);
    // TODO: Implement
    return false;
}

bool PwManager::deleteEntry(DWORD dwIndex)
{
    Q_UNUSED(dwIndex);
    // TODO: Implement
    return false;
}

bool PwManager::deleteGroupById(DWORD uGroupId, bool bCreateBackupEntries)
{
    Q_UNUSED(uGroupId);
    Q_UNUSED(bCreateBackupEntries);
    // TODO: Implement
    return false;
}

void PwManager::sortGroup(DWORD idGroup, DWORD dwSortByField)
{
    Q_UNUSED(idGroup);
    Q_UNUSED(dwSortByField);
    // TODO: Implement
}

void PwManager::sortGroupList()
{
    // TODO: Implement
}

void PwManager::fixGroupTree()
{
    // TODO: Implement
}

// More stub implementations continue...
// Additional methods will be implemented as the project progresses
