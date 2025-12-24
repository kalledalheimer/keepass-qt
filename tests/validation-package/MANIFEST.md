# KeePass Qt Validation Suite - Content Manifest

## Overview

This document provides **exact specifications** for all content in each of the 8 test databases. Use this as a reference when validating database contents in MFC KeePass.

**Generated:** 2025-12-18
**Generator Version:** 1.0
**Format:** KDB v1.x

---

## Database 1: test-empty-aes.kdb

**Purpose:** Minimal valid KDB structure
**Encryption:** AES-256-CBC
**Key Rounds:** 600,000
**Master Password:** `EmptyPass123`

### Groups (1)
| ID | Name | Image | Entries |
|----|------|-------|---------|
| 1 | Backup | 1 | 0 |

### Entries (0)
*No entries*

### Technical Notes
- Tests minimal database structure
- KDB format requires at least one group
- Empty group with default properties

---

## Database 2: test-simple-aes.kdb

**Purpose:** Basic single-entry database (AES)
**Encryption:** AES-256-CBC
**Key Rounds:** 600,000
**Master Password:** `SimplePass123`

### Groups (1)
| ID | Name | Image | Entries |
|----|------|-------|---------|
| 1 | General | 1 | 1 |

### Entries (1)

#### Entry: Sample Entry
| Field | Value |
|-------|-------|
| **Group ID** | 1 (General) |
| **Title** | Sample Entry |
| **Username** | testuser |
| **Password** | TestPass456 |
| **URL** | https://example.com |
| **Notes** | This is a test note |
| **Image ID** | 0 |
| **Binary Attachment** | None |

### Technical Notes
- All standard fields populated
- ASCII-only content
- Default image icons

---

## Database 3: test-simple-twofish.kdb

**Purpose:** Twofish encryption algorithm test
**Encryption:** Twofish (256-bit)
**Key Rounds:** 600,000
**Master Password:** `TwofishPass123`

### Groups (1)
| ID | Name | Image | Entries |
|----|------|-------|---------|
| 1 | General | 1 | 1 |

### Entries (1)

#### Entry: Twofish Test
| Field | Value |
|-------|-------|
| **Group ID** | 1 (General) |
| **Title** | Twofish Test |
| **Username** | tfuser |
| **Password** | TwoFish789 |
| **URL** | https://twofish.test |
| **Notes** | Testing Twofish encryption |
| **Image ID** | 0 |
| **Binary Attachment** | None |

### Technical Notes
- Tests Twofish algorithm (alternative to AES)
- Structure identical to simple-aes
- Verifies algorithm selection works correctly

---

## Database 4: test-complex-aes.kdb

**Purpose:** Multiple groups and entries with hierarchy
**Encryption:** AES-256-CBC
**Key Rounds:** 600,000
**Master Password:** `ComplexPass123`

### Groups (3)
| ID | Name | Image | Entries |
|----|------|-------|---------|
| 1 | Personal | 1 | 3 |
| 2 | Work | 2 | 3 |
| 3 | Banking | 3 | 4 |

### Entries (10)

#### Group 1: Personal

**Entry 1: Email Account**
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | Email Account |
| **Username** | user@email.com |
| **Password** | EmailPass123 |
| **URL** | https://mail.example.com |
| **Notes** | Notes for Email Account |

**Entry 2: Social Media**
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | Social Media |
| **Username** | social_user |
| **Password** | Social456 |
| **URL** | https://social.example.com |
| **Notes** | Notes for Social Media |

**Entry 3: Cloud Storage**
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | Cloud Storage |
| **Username** | cloud_user |
| **Password** | CloudPass789 |
| **URL** | https://storage.example.com |
| **Notes** | Notes for Cloud Storage |

#### Group 2: Work

**Entry 4: Corporate Email**
| Field | Value |
|-------|-------|
| **Group ID** | 2 |
| **Title** | Corporate Email |
| **Username** | john.doe@company.com |
| **Password** | WorkPass111 |
| **URL** | https://mail.company.com |
| **Notes** | Notes for Corporate Email |

**Entry 5: Project Management**
| Field | Value |
|-------|-------|
| **Group ID** | 2 |
| **Title** | Project Management |
| **Username** | jdoe |
| **Password** | Project222 |
| **URL** | https://pm.company.com |
| **Notes** | Notes for Project Management |

**Entry 6: VPN Access**
| Field | Value |
|-------|-------|
| **Group ID** | 2 |
| **Title** | VPN Access |
| **Username** | john.doe |
| **Password** | VPN333 |
| **URL** | https://vpn.company.com |
| **Notes** | Notes for VPN Access |

#### Group 3: Banking

**Entry 7: Main Bank**
| Field | Value |
|-------|-------|
| **Group ID** | 3 |
| **Title** | Main Bank |
| **Username** | customer12345 |
| **Password** | Bank444 |
| **URL** | https://bank.example.com |
| **Notes** | Notes for Main Bank |

**Entry 8: Credit Card**
| Field | Value |
|-------|-------|
| **Group ID** | 3 |
| **Title** | Credit Card |
| **Username** | ccuser |
| **Password** | Card555 |
| **URL** | https://creditcard.example.com |
| **Notes** | Notes for Credit Card |

**Entry 9: Investment**
| Field | Value |
|-------|-------|
| **Group ID** | 3 |
| **Title** | Investment |
| **Username** | investor |
| **Password** | Invest666 |
| **URL** | https://invest.example.com |
| **Notes** | Notes for Investment |

**Entry 10: Savings**
| Field | Value |
|-------|-------|
| **Group ID** | 3 |
| **Title** | Savings |
| **Username** | saver |
| **Password** | Save777 |
| **URL** | https://savings.example.com |
| **Notes** | Notes for Savings |

### Technical Notes
- Tests multiple groups with different image IDs
- 10 entries distributed across 3 groups
- Each entry has unique, predictable data
- Tests group hierarchy and entry distribution

---

## Database 5: test-unicode.kdb

**Purpose:** International character support (UTF-8 encoding)
**Encryption:** AES-256-CBC
**Key Rounds:** 600,000
**Master Password:** `UnicodePass123`

### Groups (1)
| ID | Name | Image | Entries |
|----|------|-------|---------|
| 1 | 国际化测试 🌍 | 1 | 5 |

**Group Name Translation:** "Internationalization Test 🌍" (Chinese + globe emoji)

### Entries (5)

#### Entry 1: Chinese
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | 中文标题 |
| **Username** | 用户名 |
| **Password** | 密码123 |
| **URL** | https://unicode.test |
| **Notes** | 这是中文备注 |

**Translations:**
- Title: "Chinese Title"
- Username: "Username"
- Password: "Password123"
- Notes: "This is a Chinese note"

#### Entry 2: Arabic
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | العربية |
| **Username** | مستخدم |
| **Password** | كلمة السر |
| **URL** | https://unicode.test |
| **Notes** | ملاحظات عربية |

**Translations:**
- Title: "Arabic"
- Username: "User"
- Password: "Password"
- Notes: "Arabic notes"

#### Entry 3: Japanese
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | 日本語 |
| **Username** | ユーザー |
| **Password** | パスワード |
| **URL** | https://unicode.test |
| **Notes** | 日本語のノート |

**Translations:**
- Title: "Japanese"
- Username: "User"
- Password: "Password"
- Notes: "Japanese notes"

#### Entry 4: Russian
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | Русский |
| **Username** | пользователь |
| **Password** | пароль123 |
| **URL** | https://unicode.test |
| **Notes** | Русские заметки |

**Translations:**
- Title: "Russian"
- Username: "user"
- Password: "password123"
- Notes: "Russian notes"

#### Entry 5: Emoji
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | Emoji Test 😀 |
| **Username** | user🎉 |
| **Password** | pass🔒 |
| **URL** | https://unicode.test |
| **Notes** | Notes with emoji 🚀✨🌈 |

**Emojis Used:**
- 😀 Grinning Face
- 🎉 Party Popper
- 🔒 Lock
- 🚀 Rocket
- ✨ Sparkles
- 🌈 Rainbow

### Technical Notes
- Tests UTF-8 encoding throughout database
- Covers major writing systems (CJK, Arabic, Cyrillic, Latin)
- Tests emoji support (multi-byte Unicode)
- Group name also contains Unicode
- All entries use same URL to simplify validation

---

## Database 6: test-attachment.kdb

**Purpose:** Binary attachment handling
**Encryption:** AES-256-CBC
**Key Rounds:** 600,000
**Master Password:** `AttachPass123`

### Groups (1)
| ID | Name | Image | Entries |
|----|------|-------|---------|
| 1 | Attachments | 1 | 1 |

### Entries (1)

#### Entry: File with Attachment
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | File with Attachment |
| **Username** | fileuser |
| **Password** | FilePass123 |
| **URL** | https://files.example.com |
| **Notes** | Entry with 100KB binary attachment |
| **Image ID** | 0 |
| **Binary Attachment** | Yes |

#### Binary Attachment Details
| Property | Value |
|----------|-------|
| **Filename** | test-file.bin |
| **Size** | 102,400 bytes (100 KB) |
| **Content** | Sequential bytes (0-255 repeating) |
| **Pattern** | `data[i] = i % 256` |

**Binary Data Pattern:**
```
Byte 0: 0x00
Byte 1: 0x01
Byte 2: 0x02
...
Byte 255: 0xFF
Byte 256: 0x00 (wraps around)
...
Byte 102399: 0xFF
```

### Technical Notes
- Tests binary attachment storage and retrieval
- 100 KB chosen as typical file size (not too large, not trivial)
- Predictable byte pattern for verification
- Attachment should be saveable from MFC KeePass

---

## Database 7: test-lowrounds.kdb

**Purpose:** Fast key derivation (low security for testing)
**Encryption:** AES-256-CBC
**Key Rounds:** 1,000 ⚠️
**Master Password:** `LowRoundsPass123`

### Groups (1)
| ID | Name | Image | Entries |
|----|------|-------|---------|
| 1 | General | 1 | 1 |

### Entries (1)

#### Entry: Low Rounds Test
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | Low Rounds Test |
| **Username** | lowuser |
| **Password** | LowPass123 |
| **URL** | https://low.test |
| **Notes** | Testing with 1,000 key rounds |
| **Image ID** | 0 |
| **Binary Attachment** | None |

### Technical Notes
- **Key Rounds:** 1,000 (extremely low, not recommended for production)
- **Purpose:** Test edge case of minimal key rounds
- **Expected:** Opens in < 1 second (very fast)
- **Verifies:** Low round count doesn't break file format

---

## Database 8: test-highrounds.kdb

**Purpose:** High security key derivation (extreme)
**Encryption:** AES-256-CBC
**Key Rounds:** 10,000,000 ⚠️
**Master Password:** `HighRoundsPass123`

### Groups (1)
| ID | Name | Image | Entries |
|----|------|-------|---------|
| 1 | General | 1 | 1 |

### Entries (1)

#### Entry: High Rounds Test
| Field | Value |
|-------|-------|
| **Group ID** | 1 |
| **Title** | High Rounds Test |
| **Username** | highuser |
| **Password** | HighPass123 |
| **URL** | https://high.test |
| **Notes** | Testing with 10M key rounds |
| **Image ID** | 0 |
| **Binary Attachment** | None |

### Technical Notes
- **Key Rounds:** 10,000,000 (10 million - very high security)
- **Purpose:** Test extreme key transformation rounds
- **Expected:** Opens in 2-5 minutes (very slow, NORMAL behavior)
- **Verifies:** High round count works correctly
- **Generation Time:** Generator took 2-5 minutes to create this file

---

## Field Specifications

### All Entries Include:
- **UUID:** 16-byte random identifier (automatically generated)
- **Creation Time:** Set to "Never expires" sentinel value
- **Last Modified Time:** Set to "Never expires" sentinel value
- **Last Access Time:** Set to "Never expires" sentinel value
- **Expiry Time:** Set to "Never expires" sentinel value

### Default Values:
- **Image ID:** 0 (default icon) unless specified
- **Tree Level:** 0 (flat hierarchy for groups)
- **Group Flags:** 0 (no special flags)

### String Encoding:
- **Format:** UTF-8
- **Termination:** Null-terminated
- **Length:** Stored separately for passwords

---

## Validation Checksums

### File Sizes (Approximate)

| Database | Approximate Size |
|----------|------------------|
| test-empty-aes.kdb | ~200 bytes |
| test-simple-aes.kdb | ~500 bytes |
| test-simple-twofish.kdb | ~500 bytes |
| test-complex-aes.kdb | ~2 KB |
| test-unicode.kdb | ~1 KB |
| test-attachment.kdb | ~103 KB |
| test-lowrounds.kdb | ~500 bytes |
| test-highrounds.kdb | ~500 bytes |

**Note:** Exact sizes vary due to encryption padding and header data.

---

## Technical Details

### KDB v1.x Header (All Files)

Each database file contains a 124-byte header with:

| Offset | Size | Field | Value |
|--------|------|-------|-------|
| 0 | 4 | Signature 1 | 0x9AA2D903 |
| 4 | 4 | Signature 2 | 0xB54BFB65 |
| 8 | 4 | Flags | 0x00000001 (AES) or 0x00000002 (Twofish) |
| 12 | 4 | Version | 0x00030004 |
| 16 | 16 | Master Seed | Random |
| 32 | 16 | Encryption IV | Random |
| 48 | 4 | Groups Count | (varies) |
| 52 | 4 | Entries Count | (varies) |
| 56 | 32 | Contents Hash | SHA-256 of content |
| 88 | 32 | Master Seed 2 | Random (for key transform) |
| 120 | 4 | Key Enc Rounds | (varies by test) |

### Encryption Details

**AES-256-CBC:**
- Key Size: 32 bytes (256 bits)
- Block Size: 16 bytes
- Mode: CBC (Cipher Block Chaining)
- Padding: PKCS#7

**Twofish:**
- Key Size: 32 bytes (256 bits)
- Block Size: 16 bytes
- Mode: CBC
- Padding: PKCS#7

### Key Derivation

**Algorithm:** AES-ECB transformation
1. Password → SHA-256 → 32-byte key
2. Transform key `N` times using Master Seed 2
3. Hash transformed key → SHA-256
4. Combine with Master Seed → SHA-256 → final key

**Rounds by Test:**
- Tests 1-3, 4-6: 600,000 rounds (default)
- Test 7: 1,000 rounds (low)
- Test 8: 10,000,000 rounds (high)

---

## Usage Notes

### For Automated Testing
- Parse this manifest to generate expected values
- Compare database contents against specifications
- Verify field-by-field match

### For Manual Testing
- Use VALIDATION.md as primary guide
- Refer to this manifest for exact field values
- Cross-reference if discrepancies found

### For Debugging
- If a test fails, compare actual vs. manifest specification
- Check each field systematically
- Verify encoding, encryption algorithm, key rounds

---

**Document Version:** 1.0
**Generated:** 2025-12-18
**Generator:** Qt KeePass Validation Suite Generator
**Maintainer:** Qt KeePass Migration Project
