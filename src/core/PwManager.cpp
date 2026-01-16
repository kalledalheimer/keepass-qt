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
#include "crypto/Rijndael.h"
#include "crypto/TwofishClass.h"
#include "util/Random.h"
#include "util/MemUtil.h"
#include "util/PwUtil.h"
#include "PwConstants.h"
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QRegularExpression>
#include <cstring>
#include <cstdlib>

// Initial allocation sizes
namespace {
    constexpr quint32 INITIAL_ENTRIES = 256;
    constexpr quint32 INITIAL_GROUPS = 32;
    constexpr DWORD DWORD_MAX = 0xFFFFFFFF;  // Maximum value for DWORD (quint32)
}

PwManager::PwManager()
    : m_pEntries(nullptr)
    , m_maxEntries(0)
    , m_numEntries(0)
    , m_pGroups(nullptr)
    , m_maxGroups(0)
    , m_numGroups(0)
    , m_pLastEditedEntry(nullptr)
    , m_nAlgorithm(ALGO_AES)
    , m_keyEncRounds(PWM_STD_KEYENCROUNDS)
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
    std::memset(m_sessionKey, 0, PWM_SESSION_KEY_SIZE);
    std::memset(m_masterKey, 0, 32);
    std::memset(m_transformedMasterKey, 0, 32);

    // Initialize UUID arrays
    std::memset(m_aLastSelectedEntryUuid, 0, 16);
    std::memset(m_aLastTopVisibleEntryUuid, 0, 16);

    m_dwLastSelectedGroupId = 0;
    m_dwLastTopVisibleGroupId = 0;

    // Generate session key for in-memory password encryption
    Random::fillBuffer(m_sessionKey, PWM_SESSION_KEY_SIZE);
}

PwManager::~PwManager()
{
    cleanUp();

    // Securely erase sensitive data
    MemUtil::mem_erase(m_sessionKey, PWM_SESSION_KEY_SIZE);
    MemUtil::mem_erase(m_masterKey, 32);
    MemUtil::mem_erase(m_transformedMasterKey, 32);
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

void PwManager::allocEntries(quint32 uEntries)
{
    Q_ASSERT(m_pEntries == nullptr);
    Q_ASSERT(uEntries > 0);

    m_pEntries = new PW_ENTRY[uEntries];
    std::memset(m_pEntries, 0, uEntries * sizeof(PW_ENTRY));

    m_maxEntries = uEntries;
    m_numEntries = 0;
}

void PwManager::deleteEntryList(bool bFreeStrings)
{
    if (m_pEntries == nullptr)
        return;

    if (bFreeStrings) {
        for (quint32 i = 0; i < m_numEntries; ++i) {
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
    m_maxEntries = 0;
    m_numEntries = 0;
}

void PwManager::allocGroups(quint32 uGroups)
{
    Q_ASSERT(m_pGroups == nullptr);
    Q_ASSERT(uGroups > 0);

    m_pGroups = new PW_GROUP[uGroups];
    std::memset(m_pGroups, 0, uGroups * sizeof(PW_GROUP));

    m_maxGroups = uGroups;
    m_numGroups = 0;
}

void PwManager::deleteGroupList(bool bFreeStrings)
{
    if (m_pGroups == nullptr)
        return;

    if (bFreeStrings) {
        for (quint32 i = 0; i < m_numGroups; ++i) {
            PW_GROUP* g = &m_pGroups[i];

            // Free strings
            delete[] g->pszGroupName;

            // Zero out the structure
            std::memset(g, 0, sizeof(PW_GROUP));
        }
    }

    delete[] m_pGroups;
    m_pGroups = nullptr;
    m_maxGroups = 0;
    m_numGroups = 0;
}

quint32 PwManager::getNumberOfEntries() const
{
    return m_numEntries;
}

quint32 PwManager::getNumberOfGroups() const
{
    return m_numGroups;
}

PW_ENTRY* PwManager::getEntry(quint32 dwIndex)
{
    if (dwIndex >= m_numEntries)
        return nullptr;
    return &m_pEntries[dwIndex];
}

PW_GROUP* PwManager::getGroup(quint32 dwIndex)
{
    if (dwIndex >= m_numGroups)
        return nullptr;
    return &m_pGroups[dwIndex];
}

void PwManager::lockEntryPassword(PW_ENTRY* pEntry)
{
    if (!pEntry || !pEntry->pszPassword || pEntry->uPasswordLen == 0)
        return;

    // XOR password bytes with session key
    // This is a simple but effective in-memory encryption
    quint8* password = reinterpret_cast<quint8*>(pEntry->pszPassword);
    for (quint32 i = 0; i < pEntry->uPasswordLen; ++i) {
        password[i] ^= m_sessionKey[i % PWM_SESSION_KEY_SIZE];
    }
}

void PwManager::unlockEntryPassword(PW_ENTRY* pEntry)
{
    if (!pEntry || !pEntry->pszPassword || pEntry->uPasswordLen == 0)
        return;

    // XOR again to restore original (XOR is reversible)
    quint8* password = reinterpret_cast<quint8*>(pEntry->pszPassword);
    for (quint32 i = 0; i < pEntry->uPasswordLen; ++i) {
        password[i] ^= m_sessionKey[i % PWM_SESSION_KEY_SIZE];
    }
}

void PwManager::newDatabase()
{
    // Clean up existing data
    cleanUp();

    // Allocate initial storage
    allocEntries(INITIAL_ENTRIES);
    allocGroups(INITIAL_GROUPS);

    // Reset header
    std::memset(&m_dbLastHeader, 0, sizeof(PW_DBHEADER));
    m_dbLastHeader.dwSignature1 = PWM_DBSIG_1;
    m_dbLastHeader.dwSignature2 = PWM_DBSIG_2;
    m_dbLastHeader.dwVersion = PWM_DBVER_DW;
    m_dbLastHeader.dwKeyEncRounds = m_keyEncRounds;

    // Generate new random seeds
    Random::fillBuffer(m_dbLastHeader.aMasterSeed, 16);
    Random::fillBuffer(m_dbLastHeader.aEncryptionIV, 16);
    Random::fillBuffer(m_dbLastHeader.aMasterSeed2, 32);

    m_numEntries = 0;
    m_numGroups = 0;
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
                            const QString& secondKey, [[maybe_unused]] bool bOverwrite,
                            [[maybe_unused]] const QString& providerName)
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
        std::memcpy(m_masterKey, keyData.constData(), 32);
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

quint32 PwManager::getKeyEncRounds() const
{
    return m_keyEncRounds;
}

void PwManager::setKeyEncRounds(quint32 dwRounds)
{
    m_keyEncRounds = dwRounds;
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

QString PwManager::getDefaultUserName() const
{
    return m_strDefaultUserName;
}

void PwManager::setDefaultUserName(const QString& strUserName)
{
    m_strDefaultUserName = strUserName;
}

bool PwManager::transformMasterKey(const quint8* pKeySeed)
{
    if (!pKeySeed)
        return false;

    // Copy master key to transformed key buffer
    std::memcpy(m_transformedMasterKey, m_masterKey, 32);

    // Perform key transformation using OpenSSL
    if (!KeyTransform::transform256(m_keyEncRounds, m_transformedMasterKey, pKeySeed)) {
        MemUtil::mem_erase(m_transformedMasterKey, 32);
        return false;
    }

    // Hash the transformed key with SHA-256
    QByteArray transformedHash = SHA256::hash(
        QByteArray(reinterpret_cast<const char*>(m_transformedMasterKey), 32));

    std::memcpy(m_transformedMasterKey, transformedHash.constData(), 32);

    return true;
}

void PwManager::protectMasterKey(bool bProtectKey)
{
    // XOR master key with session key for in-memory protection
    Q_UNUSED(bProtectKey);
    for (int i = 0; i < 32; ++i) {
        m_masterKey[i] ^= m_sessionKey[i % PWM_SESSION_KEY_SIZE];
    }
}

void PwManager::protectTransformedMasterKey(bool bProtectKey)
{
    // XOR transformed master key with session key for in-memory protection
    Q_UNUSED(bProtectKey);
    for (int i = 0; i < 32; ++i) {
        m_transformedMasterKey[i] ^= m_sessionKey[i % PWM_SESSION_KEY_SIZE];
    }
}

int PwManager::openDatabase(const QString& filePath, PWDB_REPAIR_INFO* pRepair)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/Details/PwFileImpl.cpp:86-371

    if (filePath.isEmpty())
        return PWE_INVALID_PARAM;

    // Initialize repair info
    if (pRepair)
        std::memset(pRepair, 0, sizeof(PWDB_REPAIR_INFO));

    // Open file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return PWE_NOFILEACCESS_READ;

    qint64 uFileSize = file.size();
    if (uFileSize < (qint64)sizeof(PW_DBHEADER)) {
        file.close();
        return PWE_INVALID_FILEHEADER;
    }

    // Allocate memory to hold complete file (with extra buffer space)
    quint32 uAllocated = (quint32)uFileSize + 16 + 1 + 64 + 4;
    char* pVirtualFile = new char[uAllocated];
    if (!pVirtualFile) {
        file.close();
        return PWE_NO_MEM;
    }
    std::memset(&pVirtualFile[uFileSize + 16], 0, 1 + 64);

    // Read entire file into memory
    if (file.read(pVirtualFile, uFileSize) != uFileSize) {
        MemUtil::mem_erase(pVirtualFile, uAllocated);
        delete[] pVirtualFile;
        file.close();
        return PWE_FILEERROR_READ;
    }
    file.close();

    // Extract header structure
    PW_DBHEADER hdr;
    std::memcpy(&hdr, pVirtualFile, sizeof(PW_DBHEADER));

    // Check if it's a KDBX file (KeePass 2.x)
    if ((hdr.dwSignature1 == PWM_DBSIG_1_KDBX_P && hdr.dwSignature2 == PWM_DBSIG_2_KDBX_P) ||
        (hdr.dwSignature1 == PWM_DBSIG_1_KDBX_R && hdr.dwSignature2 == PWM_DBSIG_2_KDBX_R)) {
        MemUtil::mem_erase(pVirtualFile, uAllocated);
        delete[] pVirtualFile;
        m_keyEncRounds = PWM_STD_KEYENCROUNDS;
        return PWE_UNSUPPORTED_KDBX;
    }

    // Check if we can open this (KDB v1.x)
    if (hdr.dwSignature1 != PWM_DBSIG_1 || hdr.dwSignature2 != PWM_DBSIG_2) {
        MemUtil::mem_erase(pVirtualFile, uAllocated);
        delete[] pVirtualFile;
        m_keyEncRounds = PWM_STD_KEYENCROUNDS;
        return PWE_INVALID_FILESIGNATURE;
    }

    // Check version (allow minor version differences)
    if ((hdr.dwVersion & 0xFFFFFF00) != (PWM_DBVER_DW & 0xFFFFFF00)) {
        MemUtil::mem_erase(pVirtualFile, uAllocated);
        delete[] pVirtualFile;
        return PWE_INVALID_FILEHEADER;
    }

    if (hdr.dwGroups == 0) {
        MemUtil::mem_erase(pVirtualFile, uAllocated);
        delete[] pVirtualFile;
        m_keyEncRounds = PWM_STD_KEYENCROUNDS;
        return PWE_DB_EMPTY;
    }

    // Select algorithm
    if (hdr.dwFlags & PWM_FLAG_RIJNDAEL)
        m_nAlgorithm = ALGO_AES;
    else if (hdr.dwFlags & PWM_FLAG_TWOFISH)
        m_nAlgorithm = ALGO_TWOFISH;
    else {
        MemUtil::mem_erase(pVirtualFile, uAllocated);
        delete[] pVirtualFile;
        return PWE_INVALID_FILESTRUCTURE;
    }

    m_keyEncRounds = hdr.dwKeyEncRounds;

    // Generate transformed master key from master key
    if (!transformMasterKey(hdr.aMasterSeed2)) {
        MemUtil::mem_erase(pVirtualFile, uAllocated);
        delete[] pVirtualFile;
        return PWE_CRYPT_ERROR;
    }

    // Hash the master password with the salt in the file
    // Crypto library interface: UINT8 matches Rijndael/Twofish crypto library expectations
    UINT8 uFinalKey[32];
    QByteArray masterSeed = QByteArray(reinterpret_cast<const char*>(hdr.aMasterSeed), 16);
    QByteArray transformedKey = QByteArray(reinterpret_cast<const char*>(m_transformedMasterKey), 32);
    QByteArray combined = masterSeed + transformedKey;
    QByteArray finalHash = SHA256::hash(combined);
    std::memcpy(uFinalKey, finalHash.constData(), 32);

    // Verify encrypted part size is a multiple of 16 bytes
    if (pRepair == nullptr) {
        if (((uFileSize - sizeof(PW_DBHEADER)) % 16) != 0) {
            MemUtil::mem_erase(uFinalKey, 32);
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            m_keyEncRounds = PWM_STD_KEYENCROUNDS;
            return PWE_INVALID_FILESIZE;
        }
    } else {
        // Repair mode: truncate to multiple of 16
        if (((uFileSize - sizeof(PW_DBHEADER)) % 16) != 0) {
            uFileSize -= sizeof(PW_DBHEADER);
            uFileSize &= ~0xFUL;
            uFileSize += sizeof(PW_DBHEADER);
        }
        pRepair->dwOriginalGroupCount = hdr.dwGroups;
        pRepair->dwOriginalEntryCount = hdr.dwEntries;
    }

    // Decrypt the database
    quint32 uEncryptedPartSize = 0;

    if (m_nAlgorithm == ALGO_AES) {
        CRijndael aes;
        if (aes.Init(CRijndael::CBC, CRijndael::DecryptDir, uFinalKey,
                     CRijndael::Key32Bytes, hdr.aEncryptionIV) != RIJNDAEL_SUCCESS) {
            MemUtil::mem_erase(uFinalKey, 32);
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            m_keyEncRounds = PWM_STD_KEYENCROUNDS;
            return PWE_CRYPT_ERROR;
        }

        uEncryptedPartSize = (DWORD)aes.PadDecrypt(
            (UINT8*)pVirtualFile + sizeof(PW_DBHEADER),
            uFileSize - sizeof(PW_DBHEADER),
            (UINT8*)pVirtualFile + sizeof(PW_DBHEADER));
    } else if (m_nAlgorithm == ALGO_TWOFISH) {
        CTwofish twofish;
        if (!twofish.Init(uFinalKey, 32, hdr.aEncryptionIV)) {
            MemUtil::mem_erase(uFinalKey, 32);
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            m_keyEncRounds = PWM_STD_KEYENCROUNDS;
            return PWE_CRYPT_ERROR;
        }

        uEncryptedPartSize = (DWORD)twofish.PadDecrypt(
            (UINT8*)pVirtualFile + sizeof(PW_DBHEADER),
            uFileSize - sizeof(PW_DBHEADER),
            (UINT8*)pVirtualFile + sizeof(PW_DBHEADER));
    } else {
        MemUtil::mem_erase(uFinalKey, 32);
        MemUtil::mem_erase(pVirtualFile, uAllocated);
        delete[] pVirtualFile;
        return PWE_INVALID_FILESTRUCTURE;
    }

    MemUtil::mem_erase(uFinalKey, 32);

    // Check decryption success
    if (pRepair == nullptr) {
        if ((uEncryptedPartSize > 2147483446) ||
            ((uEncryptedPartSize == 0) && ((hdr.dwGroups != 0) || (hdr.dwEntries != 0)))) {
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            m_keyEncRounds = PWM_STD_KEYENCROUNDS;
            return PWE_INVALID_KEY;
        }
    }

    // Verify content hash (check if key is correct)
    if (pRepair == nullptr) {
        QByteArray decryptedData = QByteArray(pVirtualFile + sizeof(PW_DBHEADER), uEncryptedPartSize);
        QByteArray vContentsHash = SHA256::hash(decryptedData);

        if (std::memcmp(hdr.aContentsHash, vContentsHash.constData(), 32) != 0) {
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            m_keyEncRounds = PWM_STD_KEYENCROUNDS;
            return PWE_INVALID_KEY;
        }
    }

    // Create new database and initialize internal structures
    newDatabase();

    // Store header hash (without content hash field)
    hashHeaderWithoutContentHash((quint8*)pVirtualFile, m_vHeaderHash);

    // Parse groups from the decrypted data
    quint32 pos = sizeof(PW_DBHEADER);
    quint32 uCurGroup = 0;

    PW_GROUP pwGroupTemplate;
    std::memset(&pwGroupTemplate, 0, sizeof(PW_GROUP));
    PwUtil::getNeverExpireTime(&pwGroupTemplate.tExpire);

    while (uCurGroup < hdr.dwGroups) {
        char* p = &pVirtualFile[pos];

        // Check bounds
        if (pos + 2 > (quint32)uFileSize) {
            delete[] pwGroupTemplate.pszGroupName;
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            return PWE_INVALID_FILESTRUCTURE;
        }

        // KDB v1.x file format: Must be exactly 2 bytes (USHORT) for binary compatibility
        USHORT usFieldType;
        std::memcpy(&usFieldType, p, 2);
        p += 2; pos += 2;

        if (pos + 4 > (quint32)uFileSize) {
            delete[] pwGroupTemplate.pszGroupName;
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            return PWE_INVALID_FILESTRUCTURE;
        }

        // KDB v1.x file format: Must be exactly 4 bytes (DWORD) for binary compatibility
        DWORD dwFieldSize;
        std::memcpy(&dwFieldSize, p, 4);
        p += 4; pos += 4;

        if (pos + dwFieldSize > (quint32)uFileSize) {
            delete[] pwGroupTemplate.pszGroupName;
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            return PWE_INVALID_FILESTRUCTURE;
        }

        if (!readGroupField(usFieldType, dwFieldSize, (quint8*)p, &pwGroupTemplate, pRepair)) {
            delete[] pwGroupTemplate.pszGroupName;
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            return PWE_INVALID_FILESTRUCTURE;
        }

        if (usFieldType == 0xFFFF)
            ++uCurGroup;

        p += dwFieldSize;
        pos += dwFieldSize;
    }
    delete[] pwGroupTemplate.pszGroupName;

    // Parse entries from the decrypted data
    quint32 uCurEntry = 0;

    PW_ENTRY pwEntryTemplate;
    std::memset(&pwEntryTemplate, 0, sizeof(PW_ENTRY));
    PwUtil::getNeverExpireTime(&pwEntryTemplate.tExpire);

    while (uCurEntry < hdr.dwEntries) {
        char* p = &pVirtualFile[pos];

        if (pos + 2 > (quint32)uFileSize) {
            delete[] pwEntryTemplate.pszTitle;
            delete[] pwEntryTemplate.pszURL;
            delete[] pwEntryTemplate.pszUserName;
            delete[] pwEntryTemplate.pszPassword;
            delete[] pwEntryTemplate.pszAdditional;
            delete[] pwEntryTemplate.pszBinaryDesc;
            delete[] pwEntryTemplate.pBinaryData;
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            return PWE_INVALID_FILESTRUCTURE;
        }

        // KDB v1.x file format: Must be exactly 2 bytes (USHORT) for binary compatibility
        USHORT usFieldType;
        std::memcpy(&usFieldType, p, 2);
        p += 2; pos += 2;

        if (pos + 4 > (quint32)uFileSize) {
            delete[] pwEntryTemplate.pszTitle;
            delete[] pwEntryTemplate.pszURL;
            delete[] pwEntryTemplate.pszUserName;
            delete[] pwEntryTemplate.pszPassword;
            delete[] pwEntryTemplate.pszAdditional;
            delete[] pwEntryTemplate.pszBinaryDesc;
            delete[] pwEntryTemplate.pBinaryData;
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            return PWE_INVALID_FILESTRUCTURE;
        }

        // KDB v1.x file format: Must be exactly 4 bytes (DWORD) for binary compatibility
        DWORD dwFieldSize;
        std::memcpy(&dwFieldSize, p, 4);
        p += 4; pos += 4;

        if (pos + dwFieldSize > (quint32)uFileSize) {
            delete[] pwEntryTemplate.pszTitle;
            delete[] pwEntryTemplate.pszURL;
            delete[] pwEntryTemplate.pszUserName;
            delete[] pwEntryTemplate.pszPassword;
            delete[] pwEntryTemplate.pszAdditional;
            delete[] pwEntryTemplate.pszBinaryDesc;
            delete[] pwEntryTemplate.pBinaryData;
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            return PWE_INVALID_FILESTRUCTURE;
        }

        if (!readEntryField(usFieldType, dwFieldSize, (quint8*)p, &pwEntryTemplate, pRepair)) {
            delete[] pwEntryTemplate.pszTitle;
            delete[] pwEntryTemplate.pszURL;
            delete[] pwEntryTemplate.pszUserName;
            delete[] pwEntryTemplate.pszPassword;
            delete[] pwEntryTemplate.pszAdditional;
            delete[] pwEntryTemplate.pszBinaryDesc;
            delete[] pwEntryTemplate.pBinaryData;
            MemUtil::mem_erase(pVirtualFile, uAllocated);
            delete[] pVirtualFile;
            return PWE_INVALID_FILESTRUCTURE;
        }

        if (usFieldType == 0xFFFF)
            ++uCurEntry;

        p += dwFieldSize;
        pos += dwFieldSize;
    }
    delete[] pwEntryTemplate.pszTitle;
    delete[] pwEntryTemplate.pszURL;
    delete[] pwEntryTemplate.pszUserName;
    delete[] pwEntryTemplate.pszPassword;
    delete[] pwEntryTemplate.pszAdditional;
    delete[] pwEntryTemplate.pszBinaryDesc;
    delete[] pwEntryTemplate.pBinaryData;

    // Store last header
    std::memcpy(&m_dbLastHeader, &hdr, sizeof(PW_DBHEADER));

    // Erase and delete memory file
    MemUtil::mem_erase(pVirtualFile, uAllocated);
    delete[] pVirtualFile;

    // Load and remove meta-streams
    const quint32 dwRemovedStreams = loadAndRemoveAllMetaStreams(true);
    if (pRepair)
        pRepair->dwRecognizedMetaStreamCount = dwRemovedStreams;

    deleteLostEntries();
    fixGroupTree();

    return PWE_SUCCESS;
}

int PwManager::saveDatabase(const QString& filePath, quint8* pWrittenDataHash32)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/Details/PwFileImpl.cpp:373-780

    Q_ASSERT(!filePath.isEmpty());
    if (filePath.isEmpty()) {
        return PWE_INVALID_PARAM;
    }

    if (m_numGroups == 0) {
        return PWE_DB_EMPTY;
    }

    // Add all meta-streams (UI state, etc.)
    // IMPORTANT: Must be done BEFORE setting header counts below!
    addAllMetaStreams();

    //========================================================================
    // STEP 1: Calculate required file size
    //========================================================================

    quint64 fileSize = sizeof(PW_DBHEADER);

    // Extended data size (for first group)
    QByteArray extData;
    writeExtData(extData);
    fileSize += 2 + 4 + extData.size();  // field type + field size + data

    // Calculate size of all groups
    for (quint32 i = 0; i < m_numGroups; ++i) {
        fileSize += 94;  // Fixed overhead for group fields

        // Group name (UTF-8)
        QString groupName = QString::fromUtf8(m_pGroups[i].pszGroupName);
        QByteArray nameUtf8 = groupName.toUtf8();
        fileSize += nameUtf8.length() + 1;  // +1 for null terminator
    }

    // Calculate size of all entries
    for (quint32 i = 0; i < m_numEntries; ++i) {
        PW_ENTRY* entry = &m_pEntries[i];

        // Unlock password temporarily for serialization
        unlockEntryPassword(entry);

        fileSize += 134;  // Fixed overhead for entry fields

        // All string fields (UTF-8)
        QString title = QString::fromUtf8(entry->pszTitle);
        QString username = QString::fromUtf8(entry->pszUserName);
        QString url = QString::fromUtf8(entry->pszURL);
        QString password = QString::fromUtf8(entry->pszPassword);
        QString notes = QString::fromUtf8(entry->pszAdditional);
        QString binaryDesc = QString::fromUtf8(entry->pszBinaryDesc);

        fileSize += title.toUtf8().length() + 1;
        fileSize += username.toUtf8().length() + 1;
        fileSize += url.toUtf8().length() + 1;
        fileSize += password.toUtf8().length() + 1;
        fileSize += notes.toUtf8().length() + 1;
        fileSize += binaryDesc.toUtf8().length() + 1;
        fileSize += entry->uBinaryDataLen;

        // Lock password again
        lockEntryPassword(entry);
    }

    // Round up to 16-byte boundary for block cipher
    fileSize = (fileSize + 16) - (fileSize % 16);

    quint64 allocSize = fileSize + 16;
    if (allocSize > 0xFFFFFFFFULL) {
        loadAndRemoveAllMetaStreams(false);
        return PWE_NO_MEM;
    }

    //========================================================================
    // STEP 2: Allocate memory buffer
    //========================================================================

    quint32 bufferSize = static_cast<quint32>(allocSize);
    char* buffer = nullptr;
    try {
        buffer = new char[bufferSize];
        std::memset(buffer, 0, bufferSize);
    } catch (...) {
        loadAndRemoveAllMetaStreams(false);
        return PWE_NO_MEM;
    }

    //========================================================================
    // STEP 3: Build header structure
    //========================================================================

    PW_DBHEADER hdr;
    std::memset(&hdr, 0, sizeof(PW_DBHEADER));

    hdr.dwSignature1 = PWM_DBSIG_1;
    hdr.dwSignature2 = PWM_DBSIG_2;
    hdr.dwFlags = 0x04;  // PWM_FLAG_SHA2

    if (m_nAlgorithm == ALGO_AES) {
        hdr.dwFlags |= 0x02;  // PWM_FLAG_RIJNDAEL
    } else if (m_nAlgorithm == ALGO_TWOFISH) {
        hdr.dwFlags |= 0x08;  // PWM_FLAG_TWOFISH
    } else {
        delete[] buffer;
        loadAndRemoveAllMetaStreams(false);
        return PWE_INVALID_PARAM;
    }

    hdr.dwVersion = PWM_DBVER_DW;
    hdr.dwGroups = m_numGroups;
    hdr.dwEntries = m_numEntries;
    hdr.dwKeyEncRounds = m_keyEncRounds;

    // Generate random seeds and IV
    Random::fillBuffer(hdr.aMasterSeed, 16);
    Random::fillBuffer(hdr.aEncryptionIV, 16);
    Random::fillBuffer(hdr.aMasterSeed2, 32);

    // Hash header (without content hash field)
    m_vHeaderHash.resize(32);
    hashHeaderWithoutContentHash(reinterpret_cast<BYTE*>(&hdr), m_vHeaderHash);

    //========================================================================
    // STEP 4: Serialize groups
    //========================================================================

    quint32 pos = sizeof(PW_DBHEADER);  // Skip header for now

    for (quint32 i = 0; i < m_numGroups; ++i) {
        // First group gets extended data
        if (i == 0) {
            // KDB v1.x file format: fieldType (USHORT, 2 bytes) and fieldSize (DWORD, 4 bytes) must match exact binary layout
            USHORT fieldType = 0x0000;
            DWORD fieldSize = static_cast<DWORD>(extData.size());
            std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
            std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
            std::memcpy(&buffer[pos], extData.constData(), fieldSize); pos += fieldSize;
        }

        // Field 0x0001: Group ID
        USHORT fieldType = 0x0001;
        DWORD fieldSize = 4;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], &m_pGroups[i].uGroupId, 4); pos += 4;

        // Field 0x0002: Group name
        QString groupName = QString::fromUtf8(m_pGroups[i].pszGroupName);
        QByteArray nameUtf8 = groupName.toUtf8();
        fieldType = 0x0002;
        fieldSize = nameUtf8.length() + 1;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::strcpy(&buffer[pos], nameUtf8.constData()); pos += fieldSize;

        // Field 0x0003: Creation time
        quint8 compressedTime[5];
        PwUtil::packTime(&m_pGroups[i].tCreation, compressedTime);
        fieldType = 0x0003; fieldSize = 5;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], compressedTime, 5); pos += 5;

        // Field 0x0004: Last modification time
        PwUtil::packTime(&m_pGroups[i].tLastMod, compressedTime);
        fieldType = 0x0004; fieldSize = 5;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], compressedTime, 5); pos += 5;

        // Field 0x0005: Last access time
        PwUtil::packTime(&m_pGroups[i].tLastAccess, compressedTime);
        fieldType = 0x0005; fieldSize = 5;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], compressedTime, 5); pos += 5;

        // Field 0x0006: Expiration time
        PwUtil::packTime(&m_pGroups[i].tExpire, compressedTime);
        fieldType = 0x0006; fieldSize = 5;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], compressedTime, 5); pos += 5;

        // Field 0x0007: Image ID
        fieldType = 0x0007; fieldSize = 4;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], &m_pGroups[i].uImageId, 4); pos += 4;

        // Field 0x0008: Level
        fieldType = 0x0008; fieldSize = 2;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], &m_pGroups[i].usLevel, 2); pos += 2;

        // Field 0x0009: Flags
        fieldType = 0x0009; fieldSize = 4;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], &m_pGroups[i].dwFlags, 4); pos += 4;

        // Field 0xFFFF: End of group
        fieldType = 0xFFFF; fieldSize = 0;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
    }

    //========================================================================
    // STEP 5: Serialize entries
    //========================================================================

    for (quint32 i = 0; i < m_numEntries; ++i) {
        PW_ENTRY* entry = &m_pEntries[i];

        // Unlock password for serialization
        unlockEntryPassword(entry);

        // Field 0x0001: UUID
        USHORT fieldType = 0x0001;
        DWORD fieldSize = 16;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], entry->uuid, 16); pos += 16;

        // Field 0x0002: Group ID
        fieldType = 0x0002; fieldSize = 4;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], &entry->uGroupId, 4); pos += 4;

        // Field 0x0003: Image ID
        fieldType = 0x0003; fieldSize = 4;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], &entry->uImageId, 4); pos += 4;

        // Field 0x0004: Title
        QByteArray titleUtf8 = QString::fromUtf8(entry->pszTitle).toUtf8();
        fieldType = 0x0004; fieldSize = titleUtf8.length() + 1;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::strcpy(&buffer[pos], titleUtf8.constData()); pos += fieldSize;

        // Field 0x0005: URL
        QByteArray urlUtf8 = QString::fromUtf8(entry->pszURL).toUtf8();
        fieldType = 0x0005; fieldSize = urlUtf8.length() + 1;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::strcpy(&buffer[pos], urlUtf8.constData()); pos += fieldSize;

        // Field 0x0006: Username
        QByteArray userUtf8 = QString::fromUtf8(entry->pszUserName).toUtf8();
        fieldType = 0x0006; fieldSize = userUtf8.length() + 1;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::strcpy(&buffer[pos], userUtf8.constData()); pos += fieldSize;

        // Field 0x0007: Password
        QByteArray passUtf8 = QString::fromUtf8(entry->pszPassword).toUtf8();
        fieldType = 0x0007; fieldSize = passUtf8.length() + 1;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::strcpy(&buffer[pos], passUtf8.constData()); pos += fieldSize;

        // Erase password from temporary buffer
        MemUtil::mem_erase(const_cast<char*>(passUtf8.constData()), passUtf8.length());

        // Field 0x0008: Notes
        QByteArray notesUtf8 = QString::fromUtf8(entry->pszAdditional).toUtf8();
        fieldType = 0x0008; fieldSize = notesUtf8.length() + 1;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::strcpy(&buffer[pos], notesUtf8.constData()); pos += fieldSize;

        // Field 0x0009: Creation time
        quint8 compressedTime[5];
        PwUtil::packTime(&entry->tCreation, compressedTime);
        fieldType = 0x0009; fieldSize = 5;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], compressedTime, 5); pos += 5;

        // Field 0x000A: Last modification time
        PwUtil::packTime(&entry->tLastMod, compressedTime);
        fieldType = 0x000A; fieldSize = 5;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], compressedTime, 5); pos += 5;

        // Field 0x000B: Last access time
        PwUtil::packTime(&entry->tLastAccess, compressedTime);
        fieldType = 0x000B; fieldSize = 5;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], compressedTime, 5); pos += 5;

        // Field 0x000C: Expiration time
        PwUtil::packTime(&entry->tExpire, compressedTime);
        fieldType = 0x000C; fieldSize = 5;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::memcpy(&buffer[pos], compressedTime, 5); pos += 5;

        // Field 0x000D: Binary description
        QByteArray binaryDescUtf8 = QString::fromUtf8(entry->pszBinaryDesc).toUtf8();
        fieldType = 0x000D; fieldSize = binaryDescUtf8.length() + 1;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        std::strcpy(&buffer[pos], binaryDescUtf8.constData()); pos += fieldSize;

        // Field 0x000E: Binary data
        fieldType = 0x000E; fieldSize = entry->uBinaryDataLen;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;
        if (entry->pBinaryData && fieldSize > 0) {
            std::memcpy(&buffer[pos], entry->pBinaryData, fieldSize);
        }
        pos += fieldSize;

        // Field 0xFFFF: End of entry
        fieldType = 0xFFFF; fieldSize = 0;
        std::memcpy(&buffer[pos], &fieldType, 2); pos += 2;
        std::memcpy(&buffer[pos], &fieldSize, 4); pos += 4;

        // Lock password again
        lockEntryPassword(entry);
    }

    //========================================================================
    // STEP 6: Compute content hash
    //========================================================================

    SHA256::Context contentHash;
    contentHash.update(reinterpret_cast<BYTE*>(buffer) + sizeof(PW_DBHEADER),
                       pos - sizeof(PW_DBHEADER));
    contentHash.finalize(hdr.aContentsHash);

    // Copy completed header to buffer
    std::memcpy(buffer, &hdr, sizeof(PW_DBHEADER));

    //========================================================================
    // STEP 7: Derive encryption key
    //========================================================================

    // Transform master key
    if (!transformMasterKey(hdr.aMasterSeed2)) {
        MemUtil::mem_erase(buffer, bufferSize);
        delete[] buffer;
        loadAndRemoveAllMetaStreams(false);
        return PWE_CRYPT_ERROR;
    }

    // Derive final encryption key
    // Crypto library interface: BYTE matches Rijndael/Twofish crypto library expectations
    BYTE finalKey[32];
    SHA256::Context keyHash;
    keyHash.update(hdr.aMasterSeed, 16);
    keyHash.update(m_transformedMasterKey, 32);
    keyHash.finalize(finalKey);

    //========================================================================
    // STEP 8: Encrypt content
    //========================================================================

    quint32 encryptedSize = 0;

    if (m_nAlgorithm == ALGO_AES) {
        CRijndael aes;
        if (aes.Init(CRijndael::CBC, CRijndael::EncryptDir, finalKey,
                     CRijndael::Key32Bytes, hdr.aEncryptionIV) != RIJNDAEL_SUCCESS) {
            MemUtil::mem_erase(finalKey, 32);
            MemUtil::mem_erase(buffer, bufferSize);
            delete[] buffer;
            loadAndRemoveAllMetaStreams(false);
            return PWE_CRYPT_ERROR;
        }

        encryptedSize = static_cast<quint32>(aes.PadEncrypt(
            reinterpret_cast<BYTE*>(buffer) + sizeof(PW_DBHEADER),
            pos - sizeof(PW_DBHEADER),
            reinterpret_cast<BYTE*>(buffer) + sizeof(PW_DBHEADER)));

    } else if (m_nAlgorithm == ALGO_TWOFISH) {
        CTwofish twofish;
        if (!twofish.Init(finalKey, 32, hdr.aEncryptionIV)) {
            MemUtil::mem_erase(finalKey, 32);
            MemUtil::mem_erase(buffer, bufferSize);
            delete[] buffer;
            loadAndRemoveAllMetaStreams(false);
            return PWE_CRYPT_ERROR;
        }

        encryptedSize = static_cast<quint32>(twofish.PadEncrypt(
            reinterpret_cast<BYTE*>(buffer) + sizeof(PW_DBHEADER),
            pos - sizeof(PW_DBHEADER),
            reinterpret_cast<BYTE*>(buffer) + sizeof(PW_DBHEADER)));
    }

    MemUtil::mem_erase(finalKey, 32);

    // Verify encryption succeeded
    if ((encryptedSize % 16) != 0 || encryptedSize == 0) {
        MemUtil::mem_erase(buffer, bufferSize);
        delete[] buffer;
        loadAndRemoveAllMetaStreams(false);
        return PWE_CRYPT_ERROR;
    }

    //========================================================================
    // STEP 9: Write to file
    //========================================================================

    quint32 totalSize = encryptedSize + sizeof(PW_DBHEADER);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        MemUtil::mem_erase(buffer, bufferSize);
        delete[] buffer;
        loadAndRemoveAllMetaStreams(false);
        return PWE_NOFILEACCESS_WRITE;
    }

    if (file.write(buffer, totalSize) != static_cast<qint64>(totalSize)) {
        file.close();
        MemUtil::mem_erase(buffer, bufferSize);
        delete[] buffer;
        loadAndRemoveAllMetaStreams(false);
        return PWE_FILEERROR_WRITE;
    }

    file.close();

    // Optionally compute hash of written data
    if (pWrittenDataHash32 != nullptr) {
        SHA256::Context writtenHash;
        writtenHash.update(reinterpret_cast<BYTE*>(buffer), totalSize);
        writtenHash.finalize(pWrittenDataHash32);
    }

    // Backup header
    std::memcpy(&m_dbLastHeader, &hdr, sizeof(PW_DBHEADER));

    // Cleanup
    MemUtil::mem_erase(buffer, bufferSize);
    delete[] buffer;
    loadAndRemoveAllMetaStreams(false);

    return PWE_SUCCESS;
}

//=============================================================================
// Meta-streams and Extended Data
//=============================================================================

void PwManager::writeExtData(QByteArray& data)
{
    // Field 0x0001: Header hash
    writeExtDataField(data, 0x0001,
                      reinterpret_cast<const BYTE*>(m_vHeaderHash.constData()),
                      static_cast<DWORD>(m_vHeaderHash.size()));

    // Field 0x0002: Random data to prevent guessing attacks
    BYTE randomData[32];
    Random::fillBuffer(randomData, 32);
    writeExtDataField(data, 0x0002, randomData, 32);
    MemUtil::mem_erase(randomData, 32);

    // Field 0xFFFF: Terminator
    writeExtDataField(data, 0xFFFF, nullptr, 0);
}

void PwManager::writeExtDataField(QByteArray& data, quint16 usFieldType,
                                  const quint8* pData, quint32 dwFieldSize)
{
    // Append field type (2 bytes)
    data.append(reinterpret_cast<const char*>(&usFieldType), 2);

    // Append field size (4 bytes)
    data.append(reinterpret_cast<const char*>(&dwFieldSize), 4);

    // Append field data if any
    if (dwFieldSize > 0 && pData != nullptr) {
        data.append(reinterpret_cast<const char*>(pData), dwFieldSize);
    }
}

bool PwManager::addAllMetaStreams()
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp:1677-1734
    // This method adds meta-streams (special entries) that store UI state,
    // default username, database color, search history, and custom KVPs.
    // These are preserved during save/load operations.

    bool success = true;

    // UI state meta-stream
    struct SimpleUIState {
        DWORD uLastSelectedGroupId;
        DWORD uLastTopVisibleGroupId;
        BYTE aLastSelectedEntryUuid[16];
        BYTE aLastTopVisibleEntryUuid[16];
    };

    SimpleUIState uiState;
    std::memset(&uiState, 0, sizeof(SimpleUIState));
    uiState.uLastSelectedGroupId = m_dwLastSelectedGroupId;
    uiState.uLastTopVisibleGroupId = m_dwLastTopVisibleGroupId;
    std::memcpy(uiState.aLastSelectedEntryUuid, m_aLastSelectedEntryUuid, 16);
    std::memcpy(uiState.aLastTopVisibleEntryUuid, m_aLastTopVisibleEntryUuid, 16);

    success &= addMetaStream("Simple UI State",
                            reinterpret_cast<BYTE*>(&uiState),
                            sizeof(SimpleUIState));

    // Default username meta-stream
    QByteArray defaultUserUtf8 = m_strDefaultUserName.toUtf8();
    success &= addMetaStream("Default Username",
                            reinterpret_cast<BYTE*>(defaultUserUtf8.data()),
                            defaultUserUtf8.length() + 1);

    // Database color meta-stream
    quint32 colorValue = m_clr.isValid() ? m_clr.rgb() : 0xFFFFFFFF;
    success &= addMetaStream("Database Color",
                            reinterpret_cast<BYTE*>(&colorValue),
                            sizeof(quint32));

    // Search history meta-streams (stored in reverse order)
    for (int i = m_vSearchHistory.size() - 1; i >= 0; --i) {
        QByteArray historyItemUtf8 = m_vSearchHistory[i].toUtf8();
        success &= addMetaStream("Search History Item",
                                reinterpret_cast<BYTE*>(historyItemUtf8.data()),
                                historyItemUtf8.length() + 1);
    }

    // Custom KVP meta-streams (stored in reverse order)
    for (int i = m_vCustomKVPs.size() - 1; i >= 0; --i) {
        QByteArray kvpData = serializeCustomKvp(m_vCustomKVPs[i]);
        if (!kvpData.isEmpty()) {
            success &= addMetaStream("Custom KVP",
                                    reinterpret_cast<BYTE*>(kvpData.data()),
                                    kvpData.length() + 1);
        }
    }

    // Add back all unknown meta-streams (preserve unknown data)
    for (const auto& metaStream : m_vUnknownMetaStreams) {
        success &= addMetaStream(metaStream.strName,
                                const_cast<BYTE*>(reinterpret_cast<const BYTE*>(metaStream.vData.constData())),
                                metaStream.vData.size());
    }

    return success;
}

bool PwManager::addMetaStream(const QString& metaDataDesc, quint8* pData, quint32 dwLength)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp:1595-1616
    // Meta-streams are special entries with fixed field values that store metadata

    Q_ASSERT(!metaDataDesc.isEmpty());
    Q_ASSERT(pData != nullptr);

    if (pData == nullptr || dwLength == 0) {
        return true; // Nothing to add
    }

    // Database must contain at least one group
    if (m_numGroups == 0) {
        return false;
    }

    // Meta-stream "never" time: 2999-12-28 23:59:59
    static const PW_TIME pwTimeNever = { 2999, 12, 28, 23, 59, 59 };

    // Fixed field values for meta-streams
    static const char* META_BINDESC = "bin-stream";
    static const char* META_TITLE = "Meta-Info";
    static const char* META_USER = "SYSTEM";
    static const char* META_URL = "$";

    // Create entry structure
    PW_ENTRY entry;
    std::memset(&entry, 0, sizeof(PW_ENTRY));

    // Set to first group
    entry.uGroupId = m_pGroups[0].uGroupId;

    // Set binary data
    entry.pBinaryData = pData;
    entry.uBinaryDataLen = dwLength;

    // Convert description to UTF-8
    QByteArray descUtf8 = metaDataDesc.toUtf8();
    entry.pszAdditional = new char[descUtf8.length() + 1];
    std::strcpy(entry.pszAdditional, descUtf8.constData());

    // Fixed field values
    entry.pszBinaryDesc = new char[std::strlen(META_BINDESC) + 1];
    std::strcpy(entry.pszBinaryDesc, META_BINDESC);

    entry.pszTitle = new char[std::strlen(META_TITLE) + 1];
    std::strcpy(entry.pszTitle, META_TITLE);

    entry.pszUserName = new char[std::strlen(META_USER) + 1];
    std::strcpy(entry.pszUserName, META_USER);

    entry.pszURL = new char[std::strlen(META_URL) + 1];
    std::strcpy(entry.pszURL, META_URL);

    entry.pszPassword = new char[1];
    entry.pszPassword[0] = '\0';
    entry.uPasswordLen = 0;

    // Never timestamps
    entry.tCreation = pwTimeNever;
    entry.tLastMod = pwTimeNever;
    entry.tLastAccess = pwTimeNever;
    entry.tExpire = pwTimeNever;

    // Image ID = 0
    entry.uImageId = 0;

    bool result = addEntry(&entry);

    // Clean up allocated strings (addEntry makes copies)
    delete[] entry.pszAdditional;
    delete[] entry.pszBinaryDesc;
    delete[] entry.pszTitle;
    delete[] entry.pszUserName;
    delete[] entry.pszURL;
    delete[] entry.pszPassword;

    return result;
}

QByteArray PwManager::serializeCustomKvp(const CustomKvp& kvp)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp:2032-2042
    // MFC version uses RC (Remote Control) packing which is complex.
    // For Qt version, we use a simple format: "key=value"

    QString serialized = kvp.key + "=" + kvp.value;
    return serialized.toUtf8();
}

//=============================================================================
// Stub implementations for remaining methods
// These will be implemented as needed
//=============================================================================

quint32 PwManager::getNumberOfItemsInGroup(const QString& groupName) const
{
    Q_UNUSED(groupName);
    return 0;
}

quint32 PwManager::getNumberOfItemsInGroupN(quint32 idGroup) const
{
    quint32 count = 0;
    for (quint32 i = 0; i < m_numEntries; ++i) {
        if (m_pEntries[i].uGroupId == idGroup)
            count++;
    }
    return count;
}

quint32 PwManager::getEntryByGroupN(quint32 idGroup, quint32 dwIndex) const
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp CPwManager::GetEntryByGroupN (lines 639-657)
    // Returns the absolute entry index for the Nth entry in a specific group

    if (idGroup == DWORD_MAX) {
        return DWORD_MAX;
    }
    if (dwIndex >= m_numEntries) {
        return DWORD_MAX;
    }

    quint32 uEntryInGroupCounter = 0;
    for (quint32 uCurrentEntry = 0; uCurrentEntry < m_numEntries; ++uCurrentEntry) {
        if (m_pEntries[uCurrentEntry].uGroupId == idGroup) {
            if (dwIndex == uEntryInGroupCounter) {
                return uCurrentEntry;
            }
            ++uEntryInGroupCounter;
        }
    }

    return DWORD_MAX;
}

PW_ENTRY* PwManager::getEntryByUuid(const quint8* pUuid)
{
    if (!pUuid)
        return nullptr;

    for (quint32 i = 0; i < m_numEntries; ++i) {
        if (std::memcmp(m_pEntries[i].uuid, pUuid, 16) == 0)
            return &m_pEntries[i];
    }

    return nullptr;
}

PW_GROUP* PwManager::getGroupById(quint32 idGroup)
{
    for (quint32 i = 0; i < m_numGroups; ++i) {
        if (m_pGroups[i].uGroupId == idGroup)
            return &m_pGroups[i];
    }

    return nullptr;
}

quint32 PwManager::getGroupByIdN(quint32 idGroup) const
{
    for (quint32 i = 0; i < m_numGroups; ++i) {
        if (m_pGroups[i].uGroupId == idGroup)
            return i;
    }

    return static_cast<quint32>(-1);
}

quint32 PwManager::getGroupId(const QString& groupName) const
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp:733-744
    // Returns the group ID for a group with the given name (case-insensitive)

    if (groupName.isEmpty()) {
        return DWORD_MAX;
    }

    for (quint32 i = 0; i < m_numGroups; ++i) {
        QString currentName = QString::fromUtf8(m_pGroups[i].pszGroupName);
        if (currentName.compare(groupName, Qt::CaseInsensitive) == 0) {
            return m_pGroups[i].uGroupId;
        }
    }

    return DWORD_MAX;
}

quint32 PwManager::getGroupIdByIndex(quint32 uGroupIndex) const
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp:746-750
    // Returns the group ID for a group at the specified index

    if (uGroupIndex >= m_numGroups) {
        return DWORD_MAX;
    }

    return m_pGroups[uGroupIndex].uGroupId;
}

PW_ENTRY* PwManager::getLastEditedEntry()
{
    return m_pLastEditedEntry;
}

bool PwManager::addGroup(const PW_GROUP* pTemplate)
{
    Q_ASSERT(pTemplate != nullptr);
    if (pTemplate == nullptr) {
        return false;
    }

    // Copy template to local variable
    PW_GROUP groupCopy = *pTemplate;

    // Generate a new unique group ID if needed
    if (groupCopy.uGroupId == 0 || groupCopy.uGroupId == DWORD_MAX) {
        quint32 newId = 0;
        while (true) {
            Random::fillBuffer(reinterpret_cast<quint8*>(&newId), sizeof(quint32));
            if (newId == 0 || newId == DWORD_MAX) {
                continue;
            }

            // Check if this ID already exists
            bool exists = false;
            for (quint32 i = 0; i < m_numGroups; ++i) {
                if (m_pGroups[i].uGroupId == newId) {
                    exists = true;
                    break;
                }
            }

            if (!exists) {
                break;
            }
        }
        groupCopy.uGroupId = newId;
    }

    // Expand array if needed
    if (m_numGroups == m_maxGroups) {
        quint32 newMax = m_maxGroups + 8;
        PW_GROUP* newGroups = new PW_GROUP[newMax];
        std::memset(newGroups, 0, newMax * sizeof(PW_GROUP));

        // Copy existing groups
        if (m_pGroups != nullptr) {
            std::memcpy(newGroups, m_pGroups, m_numGroups * sizeof(PW_GROUP));
            delete[] m_pGroups;
        }

        m_pGroups = newGroups;
        m_maxGroups = newMax;
    }

    ++m_numGroups;
    return setGroup(m_numGroups - 1, &groupCopy);
}

bool PwManager::setGroup(quint32 dwIndex, const PW_GROUP* pTemplate)
{
    Q_ASSERT(dwIndex < m_numGroups);
    Q_ASSERT(pTemplate != nullptr);
    Q_ASSERT(pTemplate->uGroupId != 0 && pTemplate->uGroupId != DWORD_MAX);

    if (dwIndex >= m_numGroups || pTemplate == nullptr) {
        return false;
    }

    if (pTemplate->uGroupId == 0 || pTemplate->uGroupId == DWORD_MAX) {
        return false;
    }

    // Free old group name
    delete[] m_pGroups[dwIndex].pszGroupName;

    // Allocate and copy new group name
    if (pTemplate->pszGroupName) {
        size_t len = std::strlen(pTemplate->pszGroupName);
        m_pGroups[dwIndex].pszGroupName = new char[len + 1];
        std::strcpy(m_pGroups[dwIndex].pszGroupName, pTemplate->pszGroupName);
    } else {
        m_pGroups[dwIndex].pszGroupName = new char[1];
        m_pGroups[dwIndex].pszGroupName[0] = '\0';
    }

    // Copy all fields
    m_pGroups[dwIndex].uGroupId = pTemplate->uGroupId;
    m_pGroups[dwIndex].uImageId = pTemplate->uImageId;
    m_pGroups[dwIndex].usLevel = pTemplate->usLevel;
    m_pGroups[dwIndex].dwFlags = pTemplate->dwFlags;

    m_pGroups[dwIndex].tCreation = pTemplate->tCreation;
    m_pGroups[dwIndex].tLastMod = pTemplate->tLastMod;
    m_pGroups[dwIndex].tLastAccess = pTemplate->tLastAccess;
    m_pGroups[dwIndex].tExpire = pTemplate->tExpire;

    return true;
}

bool PwManager::setEntry(quint32 dwIndex, const PW_ENTRY* pTemplate)
{
    Q_ASSERT(dwIndex < m_numEntries);
    Q_ASSERT(pTemplate != nullptr);
    Q_ASSERT(pTemplate->uGroupId != 0 && pTemplate->uGroupId != DWORD_MAX);

    if (dwIndex >= m_numEntries || pTemplate == nullptr) {
        return false;
    }

    if (pTemplate->uGroupId == 0 || pTemplate->uGroupId == DWORD_MAX) {
        return false;
    }

    // Validate required fields
    if (pTemplate->pszTitle == nullptr || pTemplate->pszUserName == nullptr ||
        pTemplate->pszURL == nullptr || pTemplate->pszPassword == nullptr ||
        pTemplate->pszAdditional == nullptr) {
        return false;
    }

    PW_ENTRY* entry = &m_pEntries[dwIndex];

    // Copy UUID
    std::memcpy(entry->uuid, pTemplate->uuid, 16);
    entry->uGroupId = pTemplate->uGroupId;
    entry->uImageId = pTemplate->uImageId;

    // Free and allocate title
    delete[] entry->pszTitle;
    size_t len = std::strlen(pTemplate->pszTitle);
    entry->pszTitle = new char[len + 1];
    std::strcpy(entry->pszTitle, pTemplate->pszTitle);

    // Free and allocate username
    delete[] entry->pszUserName;
    len = std::strlen(pTemplate->pszUserName);
    entry->pszUserName = new char[len + 1];
    std::strcpy(entry->pszUserName, pTemplate->pszUserName);

    // Free and allocate URL
    delete[] entry->pszURL;
    len = std::strlen(pTemplate->pszURL);
    entry->pszURL = new char[len + 1];
    std::strcpy(entry->pszURL, pTemplate->pszURL);

    // Free and allocate password
    delete[] entry->pszPassword;
    len = std::strlen(pTemplate->pszPassword);
    entry->pszPassword = new char[len + 1];
    std::strcpy(entry->pszPassword, pTemplate->pszPassword);

    // Free and allocate additional
    delete[] entry->pszAdditional;
    len = std::strlen(pTemplate->pszAdditional);
    entry->pszAdditional = new char[len + 1];
    std::strcpy(entry->pszAdditional, pTemplate->pszAdditional);

    // Handle binary data (only if different from current)
    if (!((entry->pBinaryData == pTemplate->pBinaryData) &&
          (entry->pszBinaryDesc == pTemplate->pszBinaryDesc))) {

        // Free old binary desc
        delete[] entry->pszBinaryDesc;
        if (pTemplate->pszBinaryDesc) {
            len = std::strlen(pTemplate->pszBinaryDesc);
            entry->pszBinaryDesc = new char[len + 1];
            std::strcpy(entry->pszBinaryDesc, pTemplate->pszBinaryDesc);
        } else {
            entry->pszBinaryDesc = new char[1];
            entry->pszBinaryDesc[0] = '\0';
        }

        // Free old binary data
        delete[] entry->pBinaryData;
        if (pTemplate->pBinaryData && pTemplate->uBinaryDataLen > 0) {
            entry->pBinaryData = new BYTE[pTemplate->uBinaryDataLen];
            std::memcpy(entry->pBinaryData, pTemplate->pBinaryData, pTemplate->uBinaryDataLen);
            entry->uBinaryDataLen = pTemplate->uBinaryDataLen;
        } else {
            entry->pBinaryData = nullptr;
            entry->uBinaryDataLen = 0;
        }
    }

    // Update password length and lock it
    entry->uPasswordLen = static_cast<DWORD>(std::strlen(entry->pszPassword));
    lockEntryPassword(entry);

    // Copy timestamps
    entry->tCreation = pTemplate->tCreation;
    entry->tLastMod = pTemplate->tLastMod;
    entry->tLastAccess = pTemplate->tLastAccess;
    entry->tExpire = pTemplate->tExpire;

    // Ensure binary desc is never null
    if (entry->pszBinaryDesc == nullptr) {
        entry->pszBinaryDesc = new char[1];
        entry->pszBinaryDesc[0] = '\0';
    }

    m_pLastEditedEntry = entry;
    return true;
}

bool PwManager::addEntry(const PW_ENTRY* pTemplate)
{
    Q_ASSERT(pTemplate != nullptr);
    if (pTemplate == nullptr) {
        return false;
    }

    Q_ASSERT(pTemplate->uGroupId != 0 && pTemplate->uGroupId != DWORD_MAX);
    if (pTemplate->uGroupId == 0 || pTemplate->uGroupId == DWORD_MAX) {
        return false;
    }

    // Expand array if needed
    if (m_numEntries == m_maxEntries) {
        quint32 newMax = m_maxEntries + 32;
        PW_ENTRY* newEntries = new PW_ENTRY[newMax];
        std::memset(newEntries, 0, newMax * sizeof(PW_ENTRY));

        // Copy existing entries
        if (m_pEntries != nullptr) {
            std::memcpy(newEntries, m_pEntries, m_numEntries * sizeof(PW_ENTRY));
            delete[] m_pEntries;
        }

        m_pEntries = newEntries;
        m_maxEntries = newMax;
    }

    // Copy template to local variable
    PW_ENTRY entryCopy = *pTemplate;

    // Generate UUID if it's all zeros
    bool isZeroUuid = true;
    for (int i = 0; i < 16; ++i) {
        if (entryCopy.uuid[i] != 0) {
            isZeroUuid = false;
            break;
        }
    }

    if (isZeroUuid) {
        Random::fillBuffer(entryCopy.uuid, 16);
    }

    // Map nullptr pointers to empty strings
    static const char emptyString[] = "";
    if (entryCopy.pszTitle == nullptr) {
        entryCopy.pszTitle = const_cast<char*>(emptyString);
    }
    if (entryCopy.pszUserName == nullptr) {
        entryCopy.pszUserName = const_cast<char*>(emptyString);
    }
    if (entryCopy.pszURL == nullptr) {
        entryCopy.pszURL = const_cast<char*>(emptyString);
    }
    if (entryCopy.pszPassword == nullptr) {
        entryCopy.pszPassword = const_cast<char*>(emptyString);
    }
    if (entryCopy.pszAdditional == nullptr) {
        entryCopy.pszAdditional = const_cast<char*>(emptyString);
    }
    if (entryCopy.pszBinaryDesc == nullptr) {
        entryCopy.pszBinaryDesc = const_cast<char*>(emptyString);
    }

    ++m_numEntries;
    return setEntry(m_numEntries - 1, &entryCopy);
}

bool PwManager::backupEntry(const PW_ENTRY* pe, bool* pbGroupCreated)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp:1562-1593
    // Creates a backup copy of an entry in the "Backup" group

    if (pe == nullptr) {
        return false;
    }

    if (pbGroupCreated != nullptr) {
        *pbGroupCreated = false;
    }

    // Check if Backup group exists
    quint32 dwGroupId = getGroupId("Backup");
    if (dwGroupId == DWORD_MAX) {
        // Create Backup group
        PW_GROUP pwg;
        std::memset(&pwg, 0, sizeof(PW_GROUP));
        pwg.pszGroupName = const_cast<char*>("Backup");

        QDateTime now = QDateTime::currentDateTime();
        PwUtil::dateTimeToPwTime(now, &pwg.tCreation);
        pwg.tLastAccess = pwg.tCreation;
        pwg.tLastMod = pwg.tCreation;
        PwManager::getNeverExpireTime(&pwg.tExpire);
        pwg.uImageId = 4;  // Icon for backup group

        if (!addGroup(&pwg)) {
            return false;
        }

        if (pbGroupCreated != nullptr) {
            *pbGroupCreated = true;
        }

        dwGroupId = getGroupId("Backup");
    }

    if (dwGroupId == DWORD_MAX) {
        return false;
    }

    // Create backup copy of entry
    PW_ENTRY pwe = *pe;
    QDateTime now = QDateTime::currentDateTime();
    PwUtil::dateTimeToPwTime(now, &pwe.tLastMod);
    pwe.uGroupId = dwGroupId;
    std::memset(&pwe.uuid, 0, 16);  // Zero UUID so new one is generated

    return addEntry(&pwe);
}

bool PwManager::deleteEntry(quint32 dwIndex)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp:861-883
    // Deletes an entry at the specified index from the database

    // Validate index
    if (dwIndex >= m_numEntries) {
        return false;
    }

    // Free all dynamically allocated memory for this entry
    delete[] m_pEntries[dwIndex].pszTitle;
    delete[] m_pEntries[dwIndex].pszURL;
    delete[] m_pEntries[dwIndex].pszUserName;
    delete[] m_pEntries[dwIndex].pszPassword;
    delete[] m_pEntries[dwIndex].pszAdditional;
    delete[] m_pEntries[dwIndex].pszBinaryDesc;
    delete[] m_pEntries[dwIndex].pBinaryData;

    // If not the last entry, shift all entries after it down by one position
    if (dwIndex != (m_numEntries - 1)) {
        for (quint32 i = dwIndex; i < (m_numEntries - 1); ++i) {
            m_pEntries[i] = m_pEntries[i + 1];
        }
    }

    // Securely erase the last entry's memory
    MemUtil::mem_erase(&m_pEntries[m_numEntries - 1], sizeof(PW_ENTRY));
    --m_numEntries;

    return true;
}

bool PwManager::deleteGroupById(quint32 uGroupId, bool bCreateBackupEntries)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp:885-930
    // Deletes a group by ID and optionally backs up its entries

    // Validate that group exists
    if (getGroupById(uGroupId) == nullptr) {
        return false;
    }

    // Get backup group IDs to avoid backing up backup entries
    quint32 dwInvGroup1 = getGroupId("Backup");
    quint32 dwInvGroup2 = getGroupId("Backup (from Templates)");

    // Delete all entries in this group
    if (m_numEntries > 0) {
        quint32 i = 0;
        while (true) {
            PW_ENTRY *p = &m_pEntries[i];
            if (p->uGroupId == uGroupId) {
                // Backup entry if requested and not already in a backup group
                if (bCreateBackupEntries &&
                    (p->uGroupId != dwInvGroup1) &&
                    (p->uGroupId != dwInvGroup2)) {
                    unlockEntryPassword(p);
                    backupEntry(p, nullptr);
                    lockEntryPassword(p);
                }

                // Delete the entry
                deleteEntry(i);
                --i;  // Adjust index since we just shifted entries down
            }

            if (++i >= m_numEntries) break;
        }
    }

    // Find the group index
    quint32 inx = getGroupByIdN(uGroupId);
    if (inx == DWORD_MAX) {
        return false;
    }

    // Free dynamically allocated group name
    delete[] m_pGroups[inx].pszGroupName;

    // If not the last group, shift all groups down by one position
    if (inx != (m_numGroups - 1)) {
        for (quint32 i = inx; i < (m_numGroups - 1); ++i) {
            m_pGroups[i] = m_pGroups[i + 1];
        }
    }

    // Securely erase the last group's memory
    MemUtil::mem_erase(&m_pGroups[m_numGroups - 1], sizeof(PW_GROUP));
    --m_numGroups;

    // Fix group tree hierarchy
    fixGroupTree();

    return true;
}

void PwManager::sortGroup(quint32 idGroup, quint32 dwSortByField)
{
    Q_UNUSED(idGroup);
    Q_UNUSED(dwSortByField);
    // TODO: Implement
}

void PwManager::sortGroupList()
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp CPwManager::SortGroupList
    // Sort groups alphabetically while maintaining tree hierarchy

    if (m_numGroups <= 1) {
        return;  // Nothing to sort
    }

    // Sort groups at each level independently
    // This is a simple bubble sort that respects tree levels
    bool swapped = true;
    while (swapped) {
        swapped = false;
        for (quint32 i = 0; i < m_numGroups - 1; ++i) {
            // Only compare groups at the same level
            if (m_pGroups[i].usLevel == m_pGroups[i + 1].usLevel) {
                // Compare group names (case-insensitive)
                QString name1 = QString::fromUtf8(m_pGroups[i].pszGroupName).toLower();
                QString name2 = QString::fromUtf8(m_pGroups[i + 1].pszGroupName).toLower();

                if (name1 > name2) {
                    // Swap groups
                    PW_GROUP temp = m_pGroups[i];
                    m_pGroups[i] = m_pGroups[i + 1];
                    m_pGroups[i + 1] = temp;
                    swapped = true;
                }
            }
        }
    }
}

bool PwManager::moveGroupExDir(quint32 dwGroupId, int iDirection)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp CPwManager::MoveGroupExDir (lines 1155-1171)
    // Move a group up (-1) or down (+1) within its siblings at the same tree level

    if (dwGroupId == 0 || dwGroupId == DWORD_MAX) {
        return false;
    }

    // Find the group by ID
    quint32 groupIndex = getGroupByIdN(dwGroupId);
    if (groupIndex == DWORD_MAX) {
        return false;  // Group not found
    }

    PW_GROUP* group = &m_pGroups[groupIndex];
    quint16 currentLevel = group->usLevel;

    // Find sibling to swap with based on direction
    if (iDirection == -1) {
        // Move up: find previous sibling at same level
        for (qint32 i = static_cast<qint32>(groupIndex) - 1; i >= 0; --i) {
            if (m_pGroups[i].usLevel < currentLevel) {
                // Hit parent level, no more siblings above
                return false;
            }
            if (m_pGroups[i].usLevel == currentLevel) {
                // Found previous sibling - swap with it
                PW_GROUP temp = m_pGroups[groupIndex];
                m_pGroups[groupIndex] = m_pGroups[i];
                m_pGroups[i] = temp;
                return true;
            }
        }
        return false;  // No previous sibling found
    }
    else if (iDirection == 1) {
        // Move down: find next sibling at same level
        for (quint32 i = groupIndex + 1; i < m_numGroups; ++i) {
            if (m_pGroups[i].usLevel < currentLevel) {
                // Hit parent level or end, no more siblings below
                return false;
            }
            if (m_pGroups[i].usLevel == currentLevel) {
                // Found next sibling - swap with it
                PW_GROUP temp = m_pGroups[groupIndex];
                m_pGroups[groupIndex] = m_pGroups[i];
                m_pGroups[i] = temp;
                return true;
            }
        }
        return false;  // No next sibling found
    }

    return false;  // Invalid direction
}

void PwManager::fixGroupTree()
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp:1420-1432
    // Fixes group tree hierarchy to prevent inconsistencies
    // Ensures first group is root and each group level increases by at most 1

    if (m_numGroups == 0) {
        return;
    }

    // First group must be root
    m_pGroups[0].usLevel = 0;

    quint16 usLastLevel = 0;
    for (quint32 i = 0; i < m_numGroups; ++i) {
        // Ensure level doesn't jump by more than 1
        if (m_pGroups[i].usLevel > static_cast<quint16>(usLastLevel + 1)) {
            m_pGroups[i].usLevel = static_cast<quint16>(usLastLevel + 1);
        }

        usLastLevel = m_pGroups[i].usLevel;
    }
}

void PwManager::moveEntry(quint32 idGroup, quint32 dwFrom, quint32 dwTo)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp CPwManager::MoveEntry (lines 1173-1185)
    // Moves an entry from one position to another within the same group

    if ((dwFrom >= m_numEntries) || (dwFrom == DWORD_MAX)) {
        return;
    }
    if ((dwTo >= m_numEntries) || (dwTo == DWORD_MAX)) {
        return;
    }
    if (dwFrom == dwTo) {
        return;
    }

    // Convert relative indices (within group) to absolute indices (across all entries)
    const quint32 dwFromEx = getEntryByGroupN(idGroup, dwFrom);
    const quint32 dwToEx = getEntryByGroupN(idGroup, dwTo);

    if ((dwFromEx == DWORD_MAX) || (dwToEx == DWORD_MAX)) {
        return;
    }

    moveInternal(dwFromEx, dwToEx);
}

void PwManager::moveInternal(quint32 dwFrom, quint32 dwTo)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp CPwManager::MoveInternal (lines 1073-1093)
    // Bubbles an entry from position dwFrom to position dwTo by swapping with neighbors

    if (dwFrom == dwTo) {
        return;  // Nothing to do
    }
    if (dwFrom >= m_numEntries) {
        return;  // Invalid index
    }
    if (dwTo >= m_numEntries) {
        return;  // Invalid index
    }

    // Set moving direction
    const qint32 lDir = ((dwFrom < dwTo) ? 1 : -1);

    qint32 i = static_cast<qint32>(dwFrom);
    while (true) {
        if (i == static_cast<qint32>(dwTo)) {
            break;
        }

        // Swap current entry with next entry in direction
        PW_ENTRY pe = m_pEntries[i];
        m_pEntries[i] = m_pEntries[i + lDir];
        m_pEntries[i + lDir] = pe;

        i += lDir;
    }
}

// Helper function for string matching in search
static bool matchesSearch(const QString& text, const QString& searchString,
                          bool caseSensitive, bool useRegex, const QRegularExpression& regex)
{
    if (text.isEmpty() && searchString.isEmpty()) {
        return true;
    }

    if (text.isEmpty()) {
        return false;
    }

    if (useRegex) {
        // Use regex matching
        QRegularExpressionMatch match = regex.match(text);
        return match.hasMatch();
    } else {
        // Use simple string contains
        Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        return text.contains(searchString, cs);
    }
}

quint32 PwManager::find(const QString& findString, bool bCaseSensitive, quint32 searchFlags,
                        quint32 nStart, quint32 nEndExcl, QString* pError)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp Find method
    // Searches for entries matching the given string

    if (findString.isEmpty()) {
        if (pError) *pError = "Search string cannot be empty";
        return 0xFFFFFFFF;
    }

    if (nStart >= m_numEntries) {
        return 0xFFFFFFFF;
    }

    quint32 nEnd = (nEndExcl == 0xFFFFFFFF) ? m_numEntries : nEndExcl;
    if (nEnd > m_numEntries) {
        nEnd = m_numEntries;
    }

    // Check if regex flag is set
    bool useRegex = (searchFlags & PWMS_REGEX) != 0;
    QRegularExpression regex;

    if (useRegex) {
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
        if (!bCaseSensitive) {
            options |= QRegularExpression::CaseInsensitiveOption;
        }

        regex.setPattern(findString);
        regex.setPatternOptions(options);

        if (!regex.isValid()) {
            if (pError) *pError = QString("Invalid regular expression: %1").arg(regex.errorString());
            return 0xFFFFFFFF;
        }
    }

    // Search through entries
    for (quint32 i = nStart; i < nEnd; ++i) {
        PW_ENTRY* entry = &m_pEntries[i];
        if (!entry) continue;

        // Check each field based on search flags
        if (searchFlags & PWMF_TITLE) {
            QString title = QString::fromUtf8(entry->pszTitle);
            if (matchesSearch(title, findString, bCaseSensitive, useRegex, regex)) {
                return i;
            }
        }

        if (searchFlags & PWMF_USER) {
            QString username = QString::fromUtf8(entry->pszUserName);
            if (matchesSearch(username, findString, bCaseSensitive, useRegex, regex)) {
                return i;
            }
        }

        if (searchFlags & PWMF_URL) {
            QString url = QString::fromUtf8(entry->pszURL);
            if (matchesSearch(url, findString, bCaseSensitive, useRegex, regex)) {
                return i;
            }
        }

        if (searchFlags & PWMF_PASSWORD) {
            unlockEntryPassword(entry);
            QString password = QString::fromUtf8(entry->pszPassword);
            lockEntryPassword(entry);
            if (matchesSearch(password, findString, bCaseSensitive, useRegex, regex)) {
                return i;
            }
        }

        if (searchFlags & PWMF_ADDITIONAL) {
            QString notes = QString::fromUtf8(entry->pszAdditional);
            if (matchesSearch(notes, findString, bCaseSensitive, useRegex, regex)) {
                return i;
            }
        }

        if (searchFlags & PWMF_UUID) {
            QString uuid = PwUtil::uuidToString(entry->uuid);
            if (matchesSearch(uuid, findString, bCaseSensitive, useRegex, regex)) {
                return i;
            }
        }

        if (searchFlags & PWMF_GROUPNAME) {
            PW_GROUP* group = getGroupById(entry->uGroupId);
            if (group) {
                QString groupName = QString::fromUtf8(group->pszGroupName);
                if (matchesSearch(groupName, findString, bCaseSensitive, useRegex, regex)) {
                    return i;
                }
            }
        }
    }

    return 0xFFFFFFFF;  // Not found
}

quint32 PwManager::findEx(const QString& findString, bool bCaseSensitive, quint32 searchFlags,
                          quint32 nStart, QString* pError)
{
    // FindEx is a simplified version that searches from nStart to end
    return find(findString, bCaseSensitive, searchFlags, nStart, 0xFFFFFFFF, pError);
}

QList<quint32> PwManager::findAll(const QString& findString, bool bCaseSensitive, quint32 searchFlags,
                                   bool excludeBackups, bool excludeExpired, QString* pError)
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp _Find method
    // Finds ALL matching entries with optional filtering
    QList<quint32> results;

    if (findString.isEmpty()) {
        if (pError) *pError = "Search string cannot be empty";
        return results;
    }

    // Get current time for expiry checking
    PW_TIME currentTime;
    PwUtil::getCurrentTime(&currentTime);

    // Get IDs for backup groups
    quint32 backupGroupId = getGroupId(PWS_BACKUPGROUP);
    quint32 backupSrcGroupId = getGroupId(PWS_BACKUPGROUP_SRC);

    // Loop through all entries and find matches
    quint32 cnt = 0;
    while (cnt < m_numEntries) {
        QString error;
        quint32 foundIndex = find(findString, bCaseSensitive, searchFlags, cnt, 0xFFFFFFFF, &error);

        if (foundIndex == 0xFFFFFFFF) {
            // No more matches
            if (!error.isEmpty() && pError && pError->isEmpty()) {
                *pError = error;
            }
            break;
        }

        PW_ENTRY* entry = getEntry(foundIndex);
        if (!entry) {
            break;
        }

        // Apply filters
        bool includeEntry = true;

        // Filter: Exclude backups
        if (excludeBackups && includeEntry) {
            if (entry->uGroupId == backupGroupId || entry->uGroupId == backupSrcGroupId) {
                includeEntry = false;
            }
        }

        // Filter: Exclude expired entries
        if (excludeExpired && includeEntry) {
            if (PwUtil::compareTime(&currentTime, &entry->tExpire) > 0) {
                includeEntry = false;
            }
        }

        // Add to results if it passes all filters
        if (includeEntry) {
            results.append(foundIndex);
        }

        // Continue searching from next entry
        cnt = foundIndex + 1;
        if (cnt >= m_numEntries) {
            break;
        }
    }

    return results;
}

QList<quint32> PwManager::findExpiredEntries(bool excludeBackups, bool excludeTANs)
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp _ShowExpiredEntries method
    // Finds all entries that have expired (tExpire < current time)
    QList<quint32> results;

    // Get current time
    PW_TIME currentTime;
    PwUtil::getCurrentTime(&currentTime);

    // Get IDs for backup groups
    quint32 backupGroupId = 0, backupSrcGroupId = 0;
    if (excludeBackups) {
        backupGroupId = getGroupId(PWS_BACKUPGROUP);
        backupSrcGroupId = getGroupId(PWS_BACKUPGROUP_SRC);
    }

    // Loop through all entries
    for (quint32 i = 0; i < m_numEntries; ++i) {
        PW_ENTRY* entry = getEntry(i);
        if (!entry) continue;

        // Apply filters
        bool includeEntry = true;

        // Filter: Exclude backups
        if (excludeBackups && includeEntry) {
            if (entry->uGroupId == backupGroupId || entry->uGroupId == backupSrcGroupId) {
                includeEntry = false;
            }
        }

        // Filter: Exclude TAN entries
        if (excludeTANs && includeEntry && entry->pszTitle) {
            if (QString::fromUtf8(entry->pszTitle) == "<TAN>") {
                includeEntry = false;
            }
        }

        // Check if expired
        if (includeEntry) {
            // Entry is expired if current time > expire time
            if (PwUtil::compareTime(&currentTime, &entry->tExpire) > 0) {
                results.append(i);
            }
        }
    }

    return results;
}

QList<quint32> PwManager::findSoonToExpireEntries(int days, bool excludeBackups, bool excludeTANs)
{
    // Reference: MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp _ShowExpiredEntries method
    // Finds entries that will expire within the specified number of days
    QList<quint32> results;

    // Get current time
    PW_TIME currentTime;
    PwUtil::getCurrentTime(&currentTime);

    // Calculate date range (using MFC's date comparison method)
    // MFC formula: date = (year * 13 * 32) + (month * 32) + day
    const quint32 dwDateNow = (static_cast<quint32>(currentTime.shYear) * 13 * 32) +
                              (static_cast<quint32>(currentTime.btMonth) * 32) +
                              (static_cast<quint32>(currentTime.btDay) & 0xFF);
    const quint32 dwSoonToExpireDays = static_cast<quint32>(days);

    // Get IDs for backup groups
    quint32 backupGroupId = 0, backupSrcGroupId = 0;
    if (excludeBackups) {
        backupGroupId = getGroupId(PWS_BACKUPGROUP);
        backupSrcGroupId = getGroupId(PWS_BACKUPGROUP_SRC);
    }

    // Loop through all entries
    for (quint32 i = 0; i < m_numEntries; ++i) {
        PW_ENTRY* entry = getEntry(i);
        if (!entry) continue;

        // Apply filters
        bool includeEntry = true;

        // Filter: Exclude backups
        if (excludeBackups && includeEntry) {
            if (entry->uGroupId == backupGroupId || entry->uGroupId == backupSrcGroupId) {
                includeEntry = false;
            }
        }

        // Filter: Exclude TAN entries
        if (excludeTANs && includeEntry && entry->pszTitle) {
            if (QString::fromUtf8(entry->pszTitle) == "<TAN>") {
                includeEntry = false;
            }
        }

        // Check if expiring soon
        if (includeEntry) {
            // Calculate entry's expiration date using MFC formula
            const quint32 dwDate = (static_cast<quint32>(entry->tExpire.shYear) * 13 * 32) +
                                   (static_cast<quint32>(entry->tExpire.btMonth) * 32) +
                                   (static_cast<quint32>(entry->tExpire.btDay) & 0xFF);

            // Include if: expiration date >= today AND (expiration - today) <= days
            if ((dwDate >= dwDateNow) && ((dwDate - dwDateNow) <= dwSoonToExpireDays)) {
                results.append(i);
            }
        }
    }

    return results;
}

// Helper function to convert UTF-8 to QString (and allocate TCHAR string)
static TCHAR* utf8ToString(const BYTE* pUTF8String)
{
    if (!pUTF8String)
        return nullptr;

    QString str = QString::fromUtf8(reinterpret_cast<const char*>(pUTF8String));
    QByteArray utf8 = str.toUtf8();

    TCHAR* result = new TCHAR[utf8.length() + 1];
    std::memcpy(result, utf8.constData(), utf8.length());
    result[utf8.length()] = 0;

    return result;
}

bool PwManager::readGroupField(quint16 usFieldType, quint32 dwFieldSize,
                                const quint8* pData, PW_GROUP* pGroup, PWDB_REPAIR_INFO* pRepair)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/Details/PwFileImpl.cpp:788-850

    Q_UNUSED(pRepair);
    quint8 aCompressedTime[5];

    switch (usFieldType) {
    case GRP_FIELD_EXT_DATA: // 0x0000
        if (!readExtData(pData, dwFieldSize, pGroup, nullptr, pRepair))
            return false;
        break;

    case GRP_FIELD_ID: // 0x0001
        if (dwFieldSize != 4)
            return false;
        std::memcpy(&pGroup->uGroupId, pData, 4);
        break;

    case GRP_FIELD_NAME: // 0x0002
        if (dwFieldSize == 0)
            return false;
        delete[] pGroup->pszGroupName;
        pGroup->pszGroupName = utf8ToString(pData);
        break;

    case GRP_FIELD_CREATION: // 0x0003
        if (dwFieldSize != 5)
            return false;
        std::memcpy(aCompressedTime, pData, 5);
        PwUtil::timeToPwTime(aCompressedTime, &pGroup->tCreation);
        break;

    case GRP_FIELD_LASTMOD: // 0x0004
        if (dwFieldSize != 5)
            return false;
        std::memcpy(aCompressedTime, pData, 5);
        PwUtil::timeToPwTime(aCompressedTime, &pGroup->tLastMod);
        break;

    case GRP_FIELD_LASTACCESS: // 0x0005
        if (dwFieldSize != 5)
            return false;
        std::memcpy(aCompressedTime, pData, 5);
        PwUtil::timeToPwTime(aCompressedTime, &pGroup->tLastAccess);
        break;

    case GRP_FIELD_EXPIRE: // 0x0006
        if (dwFieldSize != 5)
            return false;
        std::memcpy(aCompressedTime, pData, 5);
        PwUtil::timeToPwTime(aCompressedTime, &pGroup->tExpire);
        break;

    case GRP_FIELD_IMAGEID: // 0x0007
        if (dwFieldSize != 4)
            return false;
        std::memcpy(&pGroup->uImageId, pData, 4);
        break;

    case GRP_FIELD_LEVEL: // 0x0008
        if (dwFieldSize != 2)
            return false;
        std::memcpy(&pGroup->usLevel, pData, 2);
        break;

    case GRP_FIELD_FLAGS: // 0x0009
        if (dwFieldSize != 4)
            return false;
        std::memcpy(&pGroup->dwFlags, pData, 4);
        break;

    case GRP_FIELD_END: // 0xFFFF
        addGroup(pGroup);
        delete[] pGroup->pszGroupName;
        pGroup->pszGroupName = nullptr;
        std::memset(pGroup, 0, sizeof(PW_GROUP));
        PwUtil::getNeverExpireTime(&pGroup->tExpire);
        break;

    default:
        // Unknown field type - ignore
        break;
    }

    return true;
}

bool PwManager::readEntryField(quint16 usFieldType, quint32 dwFieldSize,
                                const quint8* pData, PW_ENTRY* pEntry, PWDB_REPAIR_INFO* pRepair)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/Details/PwFileImpl.cpp:852-950

    Q_UNUSED(pRepair);
    quint8 aCompressedTime[5];

    switch (usFieldType) {
    case ENT_FIELD_EXT_DATA: // 0x0000
        if (!readExtData(pData, dwFieldSize, nullptr, pEntry, pRepair))
            return false;
        break;

    case ENT_FIELD_UUID: // 0x0001
        if (dwFieldSize != 16)
            return false;
        std::memcpy(pEntry->uuid, pData, 16);
        break;

    case ENT_FIELD_GROUPID: // 0x0002
        if (dwFieldSize != 4)
            return false;
        std::memcpy(&pEntry->uGroupId, pData, 4);
        break;

    case ENT_FIELD_IMAGEID: // 0x0003
        if (dwFieldSize != 4)
            return false;
        std::memcpy(&pEntry->uImageId, pData, 4);
        break;

    case ENT_FIELD_TITLE: // 0x0004
        if (dwFieldSize == 0)
            return false;
        delete[] pEntry->pszTitle;
        pEntry->pszTitle = utf8ToString(pData);
        break;

    case ENT_FIELD_URL: // 0x0005
        if (dwFieldSize == 0)
            return false;
        delete[] pEntry->pszURL;
        pEntry->pszURL = utf8ToString(pData);
        break;

    case ENT_FIELD_USERNAME: // 0x0006
        if (dwFieldSize == 0)
            return false;
        delete[] pEntry->pszUserName;
        pEntry->pszUserName = utf8ToString(pData);
        break;

    case ENT_FIELD_PASSWORD: // 0x0007
        if (dwFieldSize == 0)
            return false;
        // Securely erase old password
        if (pEntry->pszPassword) {
            MemUtil::mem_erase(pEntry->pszPassword, std::strlen(pEntry->pszPassword));
        }
        delete[] pEntry->pszPassword;
        pEntry->pszPassword = utf8ToString(pData);
        pEntry->uPasswordLen = (DWORD)std::strlen(pEntry->pszPassword);
        break;

    case ENT_FIELD_ADDITIONAL: // 0x0008
        if (dwFieldSize == 0)
            return false;
        delete[] pEntry->pszAdditional;
        pEntry->pszAdditional = utf8ToString(pData);
        break;

    case ENT_FIELD_CREATION: // 0x0009
        if (dwFieldSize != 5)
            return false;
        std::memcpy(aCompressedTime, pData, 5);
        PwUtil::timeToPwTime(aCompressedTime, &pEntry->tCreation);
        break;

    case ENT_FIELD_LASTMOD: // 0x000A
        if (dwFieldSize != 5)
            return false;
        std::memcpy(aCompressedTime, pData, 5);
        PwUtil::timeToPwTime(aCompressedTime, &pEntry->tLastMod);
        break;

    case ENT_FIELD_LASTACCESS: // 0x000B
        if (dwFieldSize != 5)
            return false;
        std::memcpy(aCompressedTime, pData, 5);
        PwUtil::timeToPwTime(aCompressedTime, &pEntry->tLastAccess);
        break;

    case ENT_FIELD_EXPIRE: // 0x000C
        if (dwFieldSize != 5)
            return false;
        std::memcpy(aCompressedTime, pData, 5);
        PwUtil::timeToPwTime(aCompressedTime, &pEntry->tExpire);
        break;

    case ENT_FIELD_BINARYDESC: // 0x000D
        if (dwFieldSize == 0)
            return false;
        delete[] pEntry->pszBinaryDesc;
        pEntry->pszBinaryDesc = utf8ToString(pData);
        break;

    case ENT_FIELD_BINARYDATA: // 0x000E
        delete[] pEntry->pBinaryData;
        pEntry->pBinaryData = nullptr;
        if (dwFieldSize != 0) {
            pEntry->pBinaryData = new BYTE[dwFieldSize];
            std::memcpy(pEntry->pBinaryData, pData, dwFieldSize);
            pEntry->uBinaryDataLen = dwFieldSize;
        } else {
            pEntry->uBinaryDataLen = 0;
        }
        break;

    case ENT_FIELD_END: // 0xFFFF
        if (dwFieldSize != 0)
            return false;
        addEntry(pEntry);
        // Clean up entry template
        if (pEntry->pszPassword) {
            MemUtil::mem_erase(pEntry->pszPassword, std::strlen(pEntry->pszPassword));
        }
        delete[] pEntry->pszTitle;
        delete[] pEntry->pszURL;
        delete[] pEntry->pszUserName;
        delete[] pEntry->pszPassword;
        delete[] pEntry->pszAdditional;
        delete[] pEntry->pszBinaryDesc;
        delete[] pEntry->pBinaryData;
        std::memset(pEntry, 0, sizeof(PW_ENTRY));
        PwUtil::getNeverExpireTime(&pEntry->tExpire);
        break;

    default:
        // Unknown field type - ignore
        break;
    }

    return true;
}

bool PwManager::readExtData(const quint8* pData, quint32 dwDataSize, PW_GROUP* pg,
                             PW_ENTRY* pe, PWDB_REPAIR_INFO* pRepair)
{
    // Reference: Extended data reading - for future implementation
    // For now, just ignore extended data
    Q_UNUSED(pData);
    Q_UNUSED(dwDataSize);
    Q_UNUSED(pg);
    Q_UNUSED(pe);
    Q_UNUSED(pRepair);

    return true; // Silently ignore extended data for now
}

void PwManager::hashHeaderWithoutContentHash(const quint8* pbHeader, QByteArray& vHash)
{
    // Hash header excluding the content hash field (bytes 56-87)
    // Reference: MFC version hashes header without content hash for verification

    QByteArray headerPart1 = QByteArray(reinterpret_cast<const char*>(pbHeader), 56);
    QByteArray headerPart2 = QByteArray(reinterpret_cast<const char*>(pbHeader + 88), 36);
    QByteArray combined = headerPart1 + headerPart2;

    vHash = SHA256::hash(combined);
}

quint32 PwManager::loadAndRemoveAllMetaStreams(bool bAcceptUnknown)
{
    // Reference: Meta-streams are special entries that store KeePass metadata
    // like custom icons, UI state, search history, etc.
    // Meta-streams are identified by:
    //   - pszBinaryDesc = "bin-stream"
    //   - pszTitle = "Meta-Info"
    //   - pszUserName = "SYSTEM"
    //   - pszURL = "$"

    Q_UNUSED(bAcceptUnknown);

    quint32 dwRemoved = 0;

    // Scan entries from back to front (to safely remove while iterating)
    for (quint32 i = m_numEntries; i > 0; --i) {
        PW_ENTRY* entry = &m_pEntries[i - 1];

        // Check if this is a meta-stream entry
        bool isMetaStream = false;
        if (entry->pszBinaryDesc && strcmp(entry->pszBinaryDesc, "bin-stream") == 0 &&
            entry->pszTitle && strcmp(entry->pszTitle, "Meta-Info") == 0 &&
            entry->pszUserName && strcmp(entry->pszUserName, "SYSTEM") == 0 &&
            entry->pszURL && strcmp(entry->pszURL, "$") == 0) {
            isMetaStream = true;
        }

        if (isMetaStream) {
            // TODO: Actually parse and load the meta-stream data
            // For now, just remove the entry

            // Free entry strings
            if (entry->pszTitle) delete[] entry->pszTitle;
            if (entry->pszUserName) delete[] entry->pszUserName;
            if (entry->pszPassword) delete[] entry->pszPassword;
            if (entry->pszURL) delete[] entry->pszURL;
            if (entry->pszAdditional) delete[] entry->pszAdditional;
            if (entry->pszBinaryDesc) delete[] entry->pszBinaryDesc;
            if (entry->pBinaryData) delete[] entry->pBinaryData;

            // Shift remaining entries down
            for (quint32 j = i - 1; j < m_numEntries - 1; ++j) {
                m_pEntries[j] = m_pEntries[j + 1];
            }

            m_numEntries--;
            dwRemoved++;
        }
    }

    return dwRemoved;
}

quint32 PwManager::deleteLostEntries()
{
    // Delete entries that reference non-existent groups
    // Reference: MFC version removes orphaned entries

    quint32 dwDeleted = 0;

    // TODO: Implement orphan entry deletion
    // For now, assume all entries are valid

    return dwDeleted;
}

// More stub implementations continue...
// Additional methods will be implemented as the project progresses
