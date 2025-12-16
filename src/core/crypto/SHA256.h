/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef SHA256_H
#define SHA256_H

#include <QByteArray>
#include "SHA2/SHA2.h"

/// Qt-friendly wrapper for SHA-256 hashing
/// Wraps the original SHA2 implementation from MFC version
class SHA256
{
public:
    /// Hash data and return 32-byte SHA-256 digest
    static QByteArray hash(const QByteArray& data)
    {
        QByteArray result(32, 0);
        sha256_ctx ctx;
        sha256_begin(&ctx);
        sha256_hash(reinterpret_cast<const unsigned char*>(data.constData()),
                    static_cast<unsigned long>(data.size()), &ctx);
        sha256_end(reinterpret_cast<unsigned char*>(result.data()), &ctx);
        return result;
    }

    /// Hash raw bytes and return 32-byte SHA-256 digest
    static void hash(const unsigned char* input, unsigned long length, unsigned char* output)
    {
        sha256_ctx ctx;
        sha256_begin(&ctx);
        sha256_hash(input, length, &ctx);
        sha256_end(output, &ctx);
    }

    /// Create a hash context for incremental hashing
    class Context
    {
    public:
        Context() { sha256_begin(&m_ctx); }

        void update(const QByteArray& data) {
            sha256_hash(reinterpret_cast<const unsigned char*>(data.constData()),
                       static_cast<unsigned long>(data.size()), &m_ctx);
        }

        void update(const unsigned char* data, unsigned long length) {
            sha256_hash(data, length, &m_ctx);
        }

        QByteArray finalize() {
            QByteArray result(32, 0);
            sha256_end(reinterpret_cast<unsigned char*>(result.data()), &m_ctx);
            return result;
        }

        void finalize(unsigned char* output) {
            sha256_end(output, &m_ctx);
        }

    private:
        sha256_ctx m_ctx;
    };
};

#endif // SHA256_H
