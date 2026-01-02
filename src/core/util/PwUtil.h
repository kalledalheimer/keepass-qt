/*
  KeePass Password Safe - Qt Port
  Password utilities
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025
*/

#ifndef PW_UTIL_H
#define PW_UTIL_H

#include "../PwStructs.h"
#include <QDateTime>

/// Password and time utility functions
class PwUtil
{
public:
    /// Convert PW_TIME to QDateTime
    static QDateTime pwTimeToDateTime(const PW_TIME* pTime);

    /// Convert QDateTime to PW_TIME
    static void dateTimeToPwTime(const QDateTime& dt, PW_TIME* pTime);

    /// Pack PW_TIME into 5 compressed bytes (for file format)
    /// Format: 2 bytes year, 1 byte month, 1 byte day, 1 byte (hour+minute/2+second/4)
    static void packTime(const PW_TIME* pTime, quint8* pBytes5);

    /// Unpack 5 compressed bytes into PW_TIME
    static void unpackTime(const quint8* pBytes5, PW_TIME* pTime);

    /// Check if a PW_TIME represents "never expire" (year 2999)
    static bool isNeverExpire(const PW_TIME* pTime);

    /// Get current time as PW_TIME
    static void getCurrentTime(PW_TIME* pTime);

    /// Get "never expire" time as PW_TIME (year 4092)
    static void getNeverExpireTime(PW_TIME* pTime);

    /// Alias for unpackTime (for compatibility with MFC code)
    static inline void timeToPwTime(const quint8* pBytes5, PW_TIME* pTime) {
        unpackTime(pBytes5, pTime);
    }

    /// Convert UUID (16 bytes) to string representation
    /// Format: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" (lowercase hex)
    static QString uuidToString(const quint8* uuid);

    /// Compare two PW_TIME structures
    /// Returns: -1 if t1 < t2, 0 if equal, 1 if t1 > t2
    static int compareTime(const PW_TIME* t1, const PW_TIME* t2);

private:
    PwUtil() = delete;  // Static class, no instantiation
};

// Special group names (matching MFC constants)
#define PWS_SEARCHGROUP "Search Results"
#define PWS_BACKUPGROUP "Backup"
#define PWS_BACKUPGROUP_SRC "Backup"

#endif // PW_UTIL_H
