/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "KeyTransform.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <QElapsedTimer>
#include <cstring>

// Use OpenSSL for AES-256 ECB encryption
bool KeyTransform::transform16(quint64 qwRounds, quint8* pBuffer16, const quint8* pKeySeed32)
{
    if (!pBuffer16 || !pKeySeed32)
        return false;

    // Create local copy for security (original pattern from MFC version)
    quint8 buffer[16];
    std::memcpy(buffer, pBuffer16, 16);

    // Initialize OpenSSL AES-256 ECB context
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return false;

    // Initialize for AES-256 ECB encryption (no padding)
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, pKeySeed32, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    // Disable padding - we're encrypting exactly 16 bytes each time
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    // Perform the key transformation rounds
    // Each round: buffer = AES-256-ECB-Encrypt(buffer, pKeySeed32)
    int outLen = 0;
    for (quint64 i = 0; i < qwRounds; ++i) {
        if (EVP_EncryptUpdate(ctx, buffer, &outLen, buffer, 16) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            std::memset(buffer, 0, 16);
            return false;
        }
    }

    // Copy result back to output buffer
    std::memcpy(pBuffer16, buffer, 16);

    // Clean up
    std::memset(buffer, 0, 16);
    EVP_CIPHER_CTX_free(ctx);

    return true;
}

bool KeyTransform::transform256(quint64 qwRounds, quint8* pBuffer32, const quint8* pKeySeed32)
{
    if (!pBuffer32 || !pKeySeed32)
        return false;

    // Create local copies of the data and key (security pattern from MFC version)
    quint8 buffer[32];
    quint8 key[32];
    std::memcpy(buffer, pBuffer32, 32);
    std::memcpy(key, pKeySeed32, 32);

    // Split the 32-byte buffer into two 16-byte halves
    // Process them in parallel using threads (matches MFC version behavior)
    KeyTransformThread leftThread(qwRounds, &buffer[0], key);
    KeyTransformThread rightThread(qwRounds, &buffer[16], key);

    // Start left thread
    leftThread.start();

    // Run right thread in current thread (matches MFC version pattern)
    rightThread.run();

    // Wait for left thread to complete
    leftThread.wait();

    // Check both succeeded
    bool success = leftThread.succeeded() && rightThread.succeeded();

    if (success) {
        // Copy transformed data back to output buffer
        std::memcpy(pBuffer32, buffer, 32);
    }

    // Securely erase local copies
    std::memset(buffer, 0, 32);
    std::memset(key, 0, 32);

    return success;
}

quint64 KeyTransform::benchmark(quint32 dwTimeMs)
{
    // Benchmark parameters
    quint8 testBuffer[16] = { 0 };
    quint8 testKey[32] = { 0 };

    // Initialize test data with some non-zero values
    for (int i = 0; i < 16; ++i)
        testBuffer[i] = static_cast<quint8>(i);
    for (int i = 0; i < 32; ++i)
        testKey[i] = static_cast<quint8>(i * 2);

    // Measure how many rounds we can do in the given time
    QElapsedTimer timer;
    timer.start();

    quint64 rounds = 0;
    const quint64 roundsPerCheck = 10000; // Check time every 10,000 rounds

    // Initialize OpenSSL context once for efficiency
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return 0;

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, testKey, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    EVP_CIPHER_CTX_set_padding(ctx, 0);

    int outLen = 0;
    while (timer.elapsed() < static_cast<qint64>(dwTimeMs)) {
        // Do a batch of rounds
        for (quint64 i = 0; i < roundsPerCheck; ++i) {
            if (EVP_EncryptUpdate(ctx, testBuffer, &outLen, testBuffer, 16) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                return rounds;
            }
        }
        rounds += roundsPerCheck;
    }

    EVP_CIPHER_CTX_free(ctx);

    // Return average rounds per thread (divide by 2 for two parallel threads)
    // This matches the MFC version behavior
    return rounds / 2;
}
