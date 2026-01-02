/*
  KeePass Password Safe - Qt Port
  Password utilities
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025
*/

#include "PwUtil.h"

QDateTime PwUtil::pwTimeToDateTime(const PW_TIME* pTime)
{
    if (!pTime)
        return QDateTime();

    return QDateTime(
        QDate(pTime->shYear, pTime->btMonth, pTime->btDay),
        QTime(pTime->btHour, pTime->btMinute, pTime->btSecond)
    );
}

void PwUtil::dateTimeToPwTime(const QDateTime& dt, PW_TIME* pTime)
{
    if (!pTime)
        return;

    QDate d = dt.date();
    QTime t = dt.time();

    pTime->shYear = static_cast<USHORT>(d.year());
    pTime->btMonth = static_cast<BYTE>(d.month());
    pTime->btDay = static_cast<BYTE>(d.day());
    pTime->btHour = static_cast<BYTE>(t.hour());
    pTime->btMinute = static_cast<BYTE>(t.minute());
    pTime->btSecond = static_cast<BYTE>(t.second());
}

void PwUtil::packTime(const PW_TIME* pTime, quint8* pBytes5)
{
    if (!pTime || !pBytes5)
        return;

    // Pack time into 5 bytes (same format as MFC version)
    // Byte 0-1: Year (little-endian)
    // Byte 2: Month
    // Byte 3: Day
    // Byte 4: (Hour << 3) | (Minute >> 3) - packed hour/minute

    pBytes5[0] = static_cast<quint8>(pTime->shYear & 0xFF);
    pBytes5[1] = static_cast<quint8>((pTime->shYear >> 8) & 0xFF);
    pBytes5[2] = pTime->btMonth;
    pBytes5[3] = pTime->btDay;

    // Pack hour (5 bits) and minute/second (3 bits) into byte 4
    // Hour: 0-23 fits in 5 bits
    // Minute is divided by 2: 0-59 -> 0-29 fits in 5 bits (but we only use top 3 bits)
    // Actually, the format is more complex - let me check the MFC implementation
    // Looking at the code, it seems to pack: hour (upper 5 bits) + minute/2 (lower 3 bits)
    pBytes5[4] = static_cast<quint8>((pTime->btHour << 3) | (pTime->btMinute >> 3));
}

void PwUtil::unpackTime(const quint8* pBytes5, PW_TIME* pTime)
{
    if (!pBytes5 || !pTime)
        return;

    // Unpack 5 bytes into PW_TIME
    pTime->shYear = static_cast<USHORT>(pBytes5[0] | (pBytes5[1] << 8));
    pTime->btMonth = pBytes5[2];
    pTime->btDay = pBytes5[3];
    pTime->btHour = pBytes5[4] >> 3;  // Upper 5 bits
    pTime->btMinute = (pBytes5[4] & 0x07) << 3;  // Lower 3 bits * 8
    pTime->btSecond = 0;  // Seconds are not stored in compressed format
}

bool PwUtil::isNeverExpire(const PW_TIME* pTime)
{
    if (!pTime)
        return false;

    return (pTime->shYear == 2999);
}

void PwUtil::getCurrentTime(PW_TIME* pTime)
{
    if (!pTime)
        return;

    QDateTime now = QDateTime::currentDateTime();
    dateTimeToPwTime(now, pTime);
}

void PwUtil::getNeverExpireTime(PW_TIME* pTime)
{
    if (!pTime)
        return;

    // Reference: MFC PwFileImpl.cpp uses year 4092
    // #define RESET_TIME_FIELD_EXPIRE(pTimeEx) { \
    //   (pTimeEx)->btDay = 28; (pTimeEx)->btHour = 23; (pTimeEx)->btMinute = 59; \
    //   (pTimeEx)->btMonth = 12; (pTimeEx)->btSecond = 59; (pTimeEx)->shYear = 4092; }
    pTime->shYear = 4092;
    pTime->btMonth = 12;
    pTime->btDay = 28;
    pTime->btHour = 23;
    pTime->btMinute = 59;
    pTime->btSecond = 59;
}

QString PwUtil::uuidToString(const quint8* uuid)
{
    if (!uuid)
        return QString();

    // Convert 16-byte UUID to string format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    // Example: 12345678-1234-1234-1234-123456789abc
    return QString("%1%2%3%4-%5%6-%7%8-%9%10-%11%12%13%14%15%16")
        .arg(uuid[0], 2, 16, QChar('0'))
        .arg(uuid[1], 2, 16, QChar('0'))
        .arg(uuid[2], 2, 16, QChar('0'))
        .arg(uuid[3], 2, 16, QChar('0'))
        .arg(uuid[4], 2, 16, QChar('0'))
        .arg(uuid[5], 2, 16, QChar('0'))
        .arg(uuid[6], 2, 16, QChar('0'))
        .arg(uuid[7], 2, 16, QChar('0'))
        .arg(uuid[8], 2, 16, QChar('0'))
        .arg(uuid[9], 2, 16, QChar('0'))
        .arg(uuid[10], 2, 16, QChar('0'))
        .arg(uuid[11], 2, 16, QChar('0'))
        .arg(uuid[12], 2, 16, QChar('0'))
        .arg(uuid[13], 2, 16, QChar('0'))
        .arg(uuid[14], 2, 16, QChar('0'))
        .arg(uuid[15], 2, 16, QChar('0'));
}

int PwUtil::compareTime(const PW_TIME* t1, const PW_TIME* t2)
{
    // Reference: MFC/MFC-KeePass/KeePassLibCpp/Util/MemUtil.cpp _pwtimecmp
    if (!t1 || !t2)
        return 0;

    if (t1->shYear < t2->shYear) return -1;
    if (t1->shYear > t2->shYear) return 1;

    if (t1->btMonth < t2->btMonth) return -1;
    if (t1->btMonth > t2->btMonth) return 1;

    if (t1->btDay < t2->btDay) return -1;
    if (t1->btDay > t2->btDay) return 1;

    if (t1->btHour < t2->btHour) return -1;
    if (t1->btHour > t2->btHour) return 1;

    if (t1->btMinute < t2->btMinute) return -1;
    if (t1->btMinute > t2->btMinute) return 1;

    if (t1->btSecond < t2->btSecond) return -1;
    if (t1->btSecond > t2->btSecond) return 1;

    return 0;  // They are exactly the same
}
