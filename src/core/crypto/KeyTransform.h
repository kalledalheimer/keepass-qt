/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef KEY_TRANSFORM_H
#define KEY_TRANSFORM_H

#include <QtGlobal>
#include <QThread>
#include "../PwStructs.h"

/// Key transformation using AES-256 ECB mode (OpenSSL implementation)
/// This class performs the CPU-intensive key derivation function used by KeePass
/// to slow down brute-force attacks on the master password.
class KeyTransform
{
public:
    /// Transform a 16-byte buffer using qwRounds iterations of AES-256 ECB
    /// @param qwRounds Number of transformation rounds (typically 600,000)
    /// @param pBuffer16 Pointer to 16-byte buffer to transform (input/output)
    /// @param pKeySeed32 Pointer to 32-byte AES key
    /// @return true on success, false on error
    static bool transform16(quint64 qwRounds, quint8* pBuffer16, const quint8* pKeySeed32);

    /// Transform a 32-byte buffer (splits into two 16-byte halves, processes in parallel)
    /// This is the main key transformation function used by KeePass
    /// @param qwRounds Number of transformation rounds (typically 600,000)
    /// @param pBuffer32 Pointer to 32-byte buffer to transform (input/output)
    /// @param pKeySeed32 Pointer to 32-byte AES key
    /// @return true on success, false on error
    static bool transform256(quint64 qwRounds, quint8* pBuffer32, const quint8* pKeySeed32);

    /// Benchmark key transformation to determine optimal round count for given time
    /// @param dwTimeMs Target time in milliseconds
    /// @return Number of rounds that can be computed in the given time
    static quint64 benchmark(quint32 dwTimeMs);
};

/// Private worker thread for parallel key transformation
class KeyTransformThread : public QThread
{
    Q_OBJECT

public:
    KeyTransformThread(quint64 qwRounds, quint8* pBuffer16, const quint8* pKeySeed32)
        : m_qwRounds(qwRounds)
        , m_pBuffer(pBuffer16)
        , m_pKey(pKeySeed32)
        , m_bSuccess(false)
    {
    }

    void run() override
    {
        m_bSuccess = KeyTransform::transform16(m_qwRounds, m_pBuffer, m_pKey);
    }

    bool succeeded() const { return m_bSuccess; }

private:
    quint64 m_qwRounds;
    quint8* m_pBuffer;
    const quint8* m_pKey;
    bool m_bSuccess;
};

#endif // KEY_TRANSFORM_H
