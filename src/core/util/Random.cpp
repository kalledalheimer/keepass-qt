/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "Random.h"
#include <openssl/rand.h>
#include <cstring>

QByteArray Random::generateBytes(int count)
{
    if (count <= 0)
        return QByteArray();

    QByteArray result(count, 0);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(result.data()), count) != 1) {
        // Failed to generate random bytes
        result.clear();
    }

    return result;
}

bool Random::fillBuffer(quint8* buffer, size_t size)
{
    if (!buffer || size == 0)
        return false;

    return RAND_bytes(buffer, static_cast<int>(size)) == 1;
}

quint32 Random::generateUInt32()
{
    quint32 value = 0;
    fillBuffer(reinterpret_cast<quint8*>(&value), sizeof(value));
    return value;
}

quint64 Random::generateUInt64()
{
    quint64 value = 0;
    fillBuffer(reinterpret_cast<quint8*>(&value), sizeof(value));
    return value;
}

bool Random::generateUuid(quint8* uuid)
{
    if (!uuid)
        return false;

    // Generate 16 random bytes for UUID
    return fillBuffer(uuid, 16);
}

void Random::addEntropy(const void* data, size_t size)
{
    if (!data || size == 0)
        return;

    // Add the data to OpenSSL's entropy pool
    // The second parameter (entropy estimate) is set to 0.0 because
    // we don't know how much entropy the data actually contains
    RAND_add(data, static_cast<int>(size), 0.0);
}
