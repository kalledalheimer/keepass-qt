/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef RANDOM_H
#define RANDOM_H

#include <QtGlobal>
#include <QByteArray>

/// Cross-platform cryptographic random number generator
/// Uses OpenSSL RAND_bytes() for cryptographic quality randomness
class Random
{
public:
    /// Generate cryptographically secure random bytes
    /// @param count Number of random bytes to generate
    /// @return QByteArray containing random data
    static QByteArray generateBytes(int count);

    /// Fill a buffer with cryptographically secure random bytes
    /// @param buffer Pointer to buffer to fill
    /// @param size Size of buffer in bytes
    /// @return true on success, false on failure
    static bool fillBuffer(quint8* buffer, size_t size);

    /// Generate a random 32-bit unsigned integer
    /// @return Random uint32 value
    static quint32 generateUInt32();

    /// Generate a random 64-bit unsigned integer
    /// @return Random uint64 value
    static quint64 generateUInt64();

    /// Generate a random UUID (16 bytes)
    /// @param uuid Pointer to 16-byte buffer to fill with UUID
    /// @return true on success, false on failure
    static bool generateUuid(quint8* uuid);

    /// Add entropy to the random number generator pool
    /// This can be used to mix in additional randomness from user events
    /// @param data Pointer to data to add as entropy
    /// @param size Size of data in bytes
    static void addEntropy(const void* data, size_t size);
};

#endif // RANDOM_H
