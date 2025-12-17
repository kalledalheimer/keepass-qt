/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "TwofishClass.h"
#include <cstring>

static bool g_bInitialized = false;

CTwofish::CTwofish()
{
    std::memset(&m_key, 0, sizeof(m_key));
    std::memset(m_pInitVector, 0, 16);
}

CTwofish::~CTwofish()
{
    // Securely erase key material
    std::memset(&m_key, 0, sizeof(m_key));
    std::memset(m_pInitVector, 0, 16);
}

bool CTwofish::Init(const UINT8* pKey, UINT32 uKeyLen, const UINT8* initVector)
{
    if (!pKey || uKeyLen == 0)
        return false;

    if (!g_bInitialized) {
        Twofish_initialise();
        g_bInitialized = true;
    }

    Twofish_prepare_key((Twofish_Byte*)pKey, uKeyLen, &m_key);

    if (initVector)
        std::memcpy(m_pInitVector, initVector, 16);
    else
        std::memset(m_pInitVector, 0, 16);

    return true;
}

INT32 CTwofish::PadEncrypt(const UINT8* pInput, INT32 nInputOctets, UINT8* pOutBuffer)
{
    if (!pInput || nInputOctets <= 0 || !pOutBuffer)
        return 0;

    int numBlocks = nInputOctets / 16;
    UINT8 block[16];
    UINT8* iv = m_pInitVector;

    // Encrypt full blocks
    for (int i = numBlocks; i > 0; i--) {
        ((UINT32*)block)[0] = ((UINT32*)pInput)[0] ^ ((UINT32*)iv)[0];
        ((UINT32*)block)[1] = ((UINT32*)pInput)[1] ^ ((UINT32*)iv)[1];
        ((UINT32*)block)[2] = ((UINT32*)pInput)[2] ^ ((UINT32*)iv)[2];
        ((UINT32*)block)[3] = ((UINT32*)pInput)[3] ^ ((UINT32*)iv)[3];

        Twofish_encrypt(&m_key, (Twofish_Byte*)block, (Twofish_Byte*)pOutBuffer);

        iv = pOutBuffer;
        pInput += 16;
        pOutBuffer += 16;
    }

    // Handle padding (PKCS#7)
    int padLen = 16 - (nInputOctets - (16 * numBlocks));

    // Copy remaining data
    for (int i = 0; i < 16 - padLen; i++) {
        block[i] = (UINT8)(pInput[i] ^ iv[i]);
    }

    // Add padding bytes
    for (int i = 16 - padLen; i < 16; i++) {
        block[i] = (UINT8)((UINT8)padLen ^ iv[i]);
    }

    Twofish_encrypt(&m_key, (Twofish_Byte*)block, (Twofish_Byte*)pOutBuffer);

    return 16 * (numBlocks + 1);
}

INT32 CTwofish::PadDecrypt(const UINT8* pInput, INT32 nInputOctets, UINT8* pOutBuffer)
{
    if (!pInput || nInputOctets <= 0 || !pOutBuffer)
        return 0;

    if ((nInputOctets % 16) != 0)
        return -1;

    int numBlocks = nInputOctets / 16;
    UINT8 block[16];
    UINT32 iv[4];

    std::memcpy(iv, m_pInitVector, 16);

    // Decrypt all blocks except the last
    for (int i = numBlocks - 1; i > 0; i--) {
        Twofish_decrypt(&m_key, (Twofish_Byte*)pInput, (Twofish_Byte*)block);
        ((UINT32*)block)[0] ^= iv[0];
        ((UINT32*)block)[1] ^= iv[1];
        ((UINT32*)block)[2] ^= iv[2];
        ((UINT32*)block)[3] ^= iv[3];
        std::memcpy(iv, pInput, 16);
        std::memcpy(pOutBuffer, block, 16);
        pInput += 16;
        pOutBuffer += 16;
    }

    // Decrypt last block and check padding
    Twofish_decrypt(&m_key, (Twofish_Byte*)pInput, (Twofish_Byte*)block);
    ((UINT32*)block)[0] ^= iv[0];
    ((UINT32*)block)[1] ^= iv[1];
    ((UINT32*)block)[2] ^= iv[2];
    ((UINT32*)block)[3] ^= iv[3];

    int padLen = block[15];
    if (padLen <= 0 || padLen > 16)
        return -1;

    // Verify padding
    for (int i = 16 - padLen; i < 16; i++) {
        if (block[i] != padLen)
            return -1;
    }

    std::memcpy(pOutBuffer, block, 16 - padLen);

    return 16 * numBlocks - padLen;
}
