/*
  KeePass Password Safe - Qt Port
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef TWOFISH_CLASS_H
#define TWOFISH_CLASS_H

#include "../SysDefEx.h"
#include "Twofish.h"

/// Wrapper class for Twofish cipher with CBC mode and PKCS#7 padding
/// Ported from MFC CTwofish to Qt
class CTwofish
{
    Q_DISABLE_COPY(CTwofish)

public:
    CTwofish();
    ~CTwofish();

    /// Initialize the cipher with key and IV
    bool Init(const UINT8* pKey, UINT32 uKeyLen, const UINT8* initVector);

    /// Encrypt with PKCS#7 padding
    INT32 PadEncrypt(const UINT8* pInput, INT32 nInputOctets, UINT8* pOutBuffer);

    /// Decrypt with PKCS#7 padding
    INT32 PadDecrypt(const UINT8* pInput, INT32 nInputOctets, UINT8* pOutBuffer);

private:
    Twofish_key m_key;
    UINT8 m_pInitVector[16];
};

#endif // TWOFISH_CLASS_H
