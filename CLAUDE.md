# Qt-KeePass Migration - Project Context

> **For general MFC-to-Qt migration patterns**, see [MFC_TO_QT_MIGRATION_GUIDE.md](./MFC_TO_QT_MIGRATION_GUIDE.md)

This document contains **project-specific** decisions, requirements, and context for the KeePass Password Safe v1.43 migration to Qt.

---

## Project Overview

**Goal:** Migrate KeePass Password Safe v1.43 from MFC (Microsoft Foundation Classes) to Qt framework for cross-platform support while maintaining full compatibility and feature parity.

**Original Source:** `/MFC/MFC-KeePass/` - KeePass v1.43 (GPL v2+)
**Target:** `/Qt/Qt-KeePass/` - Qt-based cross-platform port

**Key Principle:** 100% compatibility with KDB v1.x file format - databases must be interoperable between MFC and Qt versions.

---

## Key Project Requirements

### 1. Database Format Compatibility (CRITICAL)

**Decision:** Maintain 100% KDB v1.x file format compatibility

**Rationale:**
- Users must be able to open existing KeePass 1.x databases without conversion
- Databases created by Qt version must open in original MFC KeePass
- No data loss or corruption during format transitions

**Implementation:**
- Copy crypto implementations (Rijndael, Twofish) as-is from MFC version
- Replicate exact serialization/deserialization logic
- Maintain byte-level compatibility with KDB v1.x specification:
  - Signatures: `PWM_DBSIG_1=0x9AA2D903`, `PWM_DBSIG_2=0xB54BFB65`
  - Version: `0x00030004`
  - Default key rounds: 600,000
  - Encryption: AES-256 CBC or Twofish
  - Hash: SHA-256

### 2. Feature Scope

**Decision:** Full feature parity with MFC version

**What This Includes:**
- All 25 dialogs from MFC version
- All import/export formats (CSV, XML, HTML, TXT, CodeWallet, Password Safe)
- Plugin system (adapted to Qt architecture)
- Auto-type functionality (platform-specific implementations)
- Password generation (character-set and pattern-based)
- Binary attachments
- Entry history and backup
- All configuration options (60+ settings)

**What This Excludes:**
- KDBX (KeePass 2.x) format support - not part of v1.x
- Features not present in KeePass v1.43

### 3. Platform Support

**Decision:** Windows, macOS, and Linux

**Cross-Platform Strategy:**
- Primary UI: Qt Widgets (works on all platforms)
- Platform abstraction layer for OS-specific features:
  - Memory locking (VirtualLock on Windows, mlock on Unix)
  - Clipboard operations
  - System notifications
  - Auto-type keyboard simulation

**Platform-Specific Implementation Areas:**
- Auto-type: SendInput (Windows), CGEvent (macOS), XTest (Linux)
- Memory protection: Windows DPAPI → platform keyring/mlock
- System integration: Tray icons, notifications, file associations

### 4. Cryptography Library

**Decision:** OpenSSL (industry standard)

**Alternatives Considered:**
- Qt Cryptographic Architecture (QCA) - Not chosen: requires plugins for some algorithms
- Botan - Not chosen: less widely available, additional dependency
- **OpenSSL** ✅ - Chosen: Industry standard, well-tested, has all needed algorithms, Qt has built-in support

**Usage:**
- Replace Windows BCrypt with OpenSSL EVP API for key transformation
- Use OpenSSL RAND_bytes() for cryptographic random number generation
- Use OPENSSL_cleanse() for secure memory zeroing
- Keep original Rijndael/Twofish/SHA-256 implementations for compatibility

### 5. Build System

**Decision:** CMake (primary), support both Qt5 and Qt6

**Configuration:**
- CMake 3.16+ minimum
- C++17 standard
- Security flags: `-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`
- Position Independent Executables (PIE)
- Qt 5.15 LTS or Qt 6.2+ LTS

**Dependencies:**
- Qt5/Qt6: Core, Gui, Widgets, (Network for updates)
- OpenSSL 1.1.1+ or 3.x
- Platform-specific: X11 libs (Linux), CoreGraphics (macOS), Windows SDK (Windows)

### 6. Migration Strategy

**Decision:** Phased, incremental approach - Core library first, then GUI

**7 Phases (26 weeks total):**

1. **Phase 1 (6 weeks):** Core Library Foundation ✅ COMPLETE
   - Port data structures, crypto, PwManager
   - Achieve KDB format compatibility
   - **Critical milestone:** Can open/save KDB files

2. **Phase 2 (2 weeks):** Platform Abstraction ✅ COMPLETE
   - Cross-platform interface layer
   - Settings migration (Registry → QSettings)

3. **Phase 3 (4 weeks):** Basic GUI ✅ COMPLETE
   - Main window, entry/group widgets, essential dialogs
   - **Critical milestone:** Functional CRUD operations

4. **Phase 4 (4 weeks):** Advanced GUI ✅ COMPLETE
   - All 25 dialogs, custom widgets

5. **Phase 5 (3 weeks):** Essential Features ✅ COMPLETE
   - Lock/Unlock, Change Master Key, CSV Import/Export
   - System Tray, Binary Attachments, Entry Duplication, Visit URL

6. **Phase 6 (3 weeks):** Auto-Type (NEXT)
   - Platform-specific keyboard simulation

7. **Phase 7 (3 weeks):** Import/Export & Plugins
   - All format handlers, Qt plugin system

8. **Phase 8 (4 weeks):** Polish & Release
   - Testing, localization, installers

**Why Core-First:**
- Database compatibility is most critical - must be proven before GUI work
- Core library is platform-neutral C++ - easier to port
- GUI can be built incrementally once core is stable
- Early validation of file format compatibility reduces risk

---

## KeePass-Specific Architecture

### Code Organization

**Three-Layer Architecture (preserved from MFC):**

1. **Core Library** (`src/core/`)
   - Pure C++ business logic
   - Platform-neutral where possible
   - Database management (PwManager)
   - Cryptography
   - Password generation
   - Import/export logic

2. **GUI Layer** (`src/gui/`)
   - Qt Widgets-based UI
   - Model/View architecture for lists/trees
   - Dialogs and custom widgets
   - Platform-independent UI logic

3. **Platform Layer** (`src/core/platform/`, `src/autotype/platform/`)
   - OS-specific implementations
   - Abstracted behind interfaces
   - Separate files per platform (Windows, Mac, Linux)

### Qt Model/View Pattern

**Decision:** Use QAbstractItemModel for entries and groups

**Implementation:**
```cpp
EntryModel : public QAbstractTableModel  // For entry list view
GroupModel : public QAbstractItemModel   // For group tree view
```

**Benefits:**
- Separation of data and presentation
- Built-in sorting, filtering
- Efficient updates (only changed items repaint)
- Standard Qt pattern

### Configuration Storage

**Decision:** QSettings (cross-platform, replaces Windows Registry)

**Mapping:**
- Registry keys (PWMKEY_*) → QSettings keys
- Organization: "KeePass"
- Application: "KeePass"
- Platform-specific storage:
  - Windows: Registry (HKEY_CURRENT_USER)
  - macOS: ~/Library/Preferences/com.keepass.KeePass.plist
  - Linux: ~/.config/KeePass/KeePass.conf

---

## Critical KDB v1.x File Format Specifications

### Header Structure (124 bytes - MUST NOT CHANGE)

```
Offset  Size  Field
0       4     dwSignature1 (0x9AA2D903)
4       4     dwSignature2 (0xB54BFB65)
8       4     dwFlags
12      4     dwVersion (0x00030004)
16      16    aMasterSeed
32      16    aEncryptionIV
48      4     dwGroups (count)
52      4     dwEntries (count)
56      32    aContentsHash (SHA-256)
88      32    aMasterSeed2 (for key transform)
120     4     dwKeyEncRounds (default: 600,000)
```

### Key Derivation (MUST BE EXACT)

1. Hash password with SHA-256 → 32 bytes
2. If key file: XOR password hash with key file hash
3. Transform key: AES-ECB encrypt key with itself `dwKeyEncRounds` times using `aMasterSeed2`
4. Hash transformed key with SHA-256
5. Hash result with `aMasterSeed` using SHA-256 → final master key (32 bytes)

### Encryption

- Algorithm: AES-256-CBC or Twofish
- IV: From header `aEncryptionIV` (16 bytes)
- Padding: PKCS#7
- Content: Serialized groups and entries

### Data Structures (MUST PRESERVE LAYOUT)

**PW_TIME (7 bytes with pragma pack(1)):**
```cpp
typedef struct _PW_TIME {
    USHORT shYear;   // 2 bytes
    BYTE btMonth;    // 1 byte
    BYTE btDay;      // 1 byte
    BYTE btHour;     // 1 byte
    BYTE btMinute;   // 1 byte
    BYTE btSecond;   // 1 byte
} PW_TIME;
```

**PW_GROUP:**
- Group ID (DWORD)
- Image ID (DWORD)
- Name (TCHAR*, null-terminated UTF-8 in file)
- Creation/LastMod/LastAccess/Expire times
- Tree level (USHORT)
- Flags (DWORD)

**PW_ENTRY:**
- UUID (16 bytes)
- Group ID (DWORD)
- Image ID (DWORD)
- Title, URL, Username, Password, Notes (TCHAR*)
- Password length (DWORD) - for memory protection
- Binary attachment (description + data + length)
- Creation/LastMod/LastAccess/Expire times

### Session Key (In-Memory Encryption)

**MFC Implementation (MUST REPLICATE):**
```cpp
// 32-byte session key, generated at startup
BYTE m_pSessionKey[32];

// Lock password: XOR each byte with session key
void LockEntryPassword(PW_ENTRY* entry) {
    for (DWORD i = 0; i < entry->uPasswordLen; ++i) {
        ((BYTE*)entry->pszPassword)[i] ^= m_sessionKey[i % 32];
    }
}

// Unlock password: XOR again (reversible)
void UnlockEntryPassword(PW_ENTRY* entry) {
    LockEntryPassword(entry); // Same operation
}
```

**Purpose:** Protect passwords in memory from memory dumps/debuggers

**Critical Files to Reference:**
- `MFC/MFC-KeePass/KeePassLibCpp/PwStructs.h` - Exact data structure layout
- `MFC/MFC-KeePass/KeePassLibCpp/Details/PwFileImpl.cpp` - Serialization logic
- `MFC/MFC-KeePass/KeePassLibCpp/Crypto/KeyTransform.cpp` - Key derivation

---

## Testing Strategy

### KDB Compatibility Tests (HIGHEST PRIORITY)

**Test Approach:**
1. Generate reference databases using MFC KeePass with various configurations
2. Open with Qt version → verify all data
3. Save with Qt version → verify MFC can open
4. Round-trip test: MFC → Qt → MFC → compare

**Test Cases:**
- Empty database
- Single entry/group
- Complex hierarchy (nested groups)
- All field types populated
- Binary attachments
- Special characters, Unicode
- Different encryption algorithms (AES, Twofish)
- Different key rounds (1,000 to 10,000,000)
- Key file authentication
- Meta-streams (custom icons, UI state)

**Validation:**
- Byte-level header comparison
- Field-by-field entry comparison
- Decrypt → compare cleartext
- Crypto primitive tests with known vectors

### Security Tests

- Memory leak detection (Valgrind)
- Secure memory zeroing verification
- Password not in cleartext dumps
- Timing attack resistance (constant-time comparison)

### Performance Tests

- Open large database (10,000+ entries)
- Key derivation speed (600,000 rounds)
- UI responsiveness
- Compare vs. MFC baseline (within 10%)

---

## Project-Specific Guidelines

### Type Usage Exceptions

While the [MFC_TO_QT_MIGRATION_GUIDE.md](./MFC_TO_QT_MIGRATION_GUIDE.md) recommends Qt types everywhere, this project has specific exceptions:

**Windows types are acceptable in:**
1. **KDB file format code** - Must match exact binary layout:
   - `PW_DBHEADER` field types
   - `PW_GROUP` and `PW_ENTRY` serialization
   - Field type identifiers in file I/O code

2. **Crypto library interfaces** - When interfacing with existing code:
   - Rijndael/AES encryption API (uses `UINT8`)
   - Twofish encryption API (uses `BYTE`)

3. **Temporary during migration** - Mark with TODO comments

**All other code MUST use Qt types** (quint8, quint16, quint32, quint64, bool)

### String Handling

**Project-specific convention:**
- QString variables: `s` prefix (e.g., `sTitle`, `sPassword`)
- char* only in file I/O: `pszFieldName` (legacy, at serialization boundary only)

See [MFC_TO_QT_MIGRATION_GUIDE.md](./MFC_TO_QT_MIGRATION_GUIDE.md#string-handling) for detailed patterns.

---

## Known Limitations & Trade-offs

### What We Keep from MFC

**Advantages:**
- ✅ Proven crypto implementations (Rijndael, Twofish)
- ✅ Stable KDB format (14+ years in production)
- ✅ Well-tested key derivation

**Disadvantages:**
- ⚠️ KDB v1.x format is older (KDBX is more modern)
- ⚠️ Some legacy code patterns

**Decision:** Keep for compatibility, don't modernize file format

### What We Change

**MFC → Qt:**
- ❌ Windows-only → ✅ Cross-platform
- ❌ MFC dialogs → ✅ Qt Widgets
- ❌ Registry → ✅ QSettings
- ❌ Windows crypto APIs → ✅ OpenSSL
- ❌ COM plugins → ✅ Qt plugin framework

### Future Considerations

**Out of Scope for v1.0:**
- KDBX (KeePass 2.x) format support
- Cloud sync
- Browser integration
- Biometric authentication
- Mobile versions

**Possible Future Versions:**
- v1.1: KDBX read support (one-way import)
- v2.0: Native KDBX with backward KDB compatibility
- v3.0: Cloud features

---

## Project Timeline & Status

**Start Date:** 2025-12-16
**Target Completion:** 26 weeks (June 2026)
**Current Status:** Phase 5 Complete (2026-01-06)

**Completed Phases:**
- ✅ Phase 1: Core Library Foundation (100% KDB compatibility achieved)
- ✅ Phase 2-Lite: Platform Foundation (PwSettings, MemoryProtection)
- ✅ Phase 3: Basic GUI (Full CRUD operations)
- ✅ Phase 4: Advanced GUI (Search, Password Generator, Database Settings, Options)
- ✅ Phase 5: Essential Features (Lock/Unlock, Change Key, CSV, Tray, Attachments, Duplicate, Visit URL)

**Current Phase:** Ready for Phase 6 (Auto-Type)

**Test Results:**
- ✅ 13/13 automated unit tests passing
- ✅ Windows validation: 7/8 full pass, 1/8 functional pass
- ✅ MFC → Qt round-trip compatibility verified

---

## Resources & References

### KeePass-Specific Documentation

- Original KeePass: https://keepass.info/
- KDB Format: https://keepass.info/help/kb/kdb.html

### MFC Source Files

**Core Library:**
- `MFC/MFC-KeePass/KeePassLibCpp/PwStructs.h` (255 lines)
- `MFC/MFC-KeePass/KeePassLibCpp/PwManager.h` (599 lines)
- `MFC/MFC-KeePass/KeePassLibCpp/PwManager.cpp` (2,132 lines)

**Crypto:**
- `MFC/MFC-KeePass/KeePassLibCpp/Crypto/Rijndael.cpp/h`
- `MFC/MFC-KeePass/KeePassLibCpp/Crypto/Twofish.cpp/h`
- `MFC/MFC-KeePass/KeePassLibCpp/Crypto/SHA2/SHA2.cpp/h`

**UI Reference:**
- `MFC/MFC-KeePass/WinGUI/PwSafeDlg.cpp` (11,903 lines) - Main window

### External Libraries

- Qt: LGPL v3 / GPL v2/v3 / Commercial
- OpenSSL: Apache 2.0 (OpenSSL 3.x) / dual license (OpenSSL 1.x)

---

## Decision Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2025-12-16 | Use KDB v1.x format (not KDBX) | User requirement: compatibility with existing databases |
| 2025-12-16 | Full feature parity (all 25 dialogs) | User requirement: complete migration, not subset |
| 2025-12-16 | Target Windows, macOS, Linux | User requirement: cross-platform support |
| 2025-12-16 | Use OpenSSL for crypto | Industry standard, well-tested, complete algorithm support |
| 2025-12-16 | CMake build system | Cross-platform, modern, Qt-compatible |
| 2025-12-16 | Qt 5.15/6.2+ LTS versions | Long-term support, stable APIs |
| 2025-12-16 | Phased migration (core-first) | Validate critical compatibility before GUI investment |
| 2026-01-06 | Reorganize documentation | Separate general MFC→Qt patterns from project-specific context |

---

## Contact & Collaboration

**Original Author:** Dominik Reichl <dominik.reichl@t-online.de>
**Project License:** GPL v2+
**Migration Project:** Independent port to Qt framework

---

*This document should be updated as new decisions are made during the migration process.*
*For general MFC-to-Qt migration patterns, see [MFC_TO_QT_MIGRATION_GUIDE.md](./MFC_TO_QT_MIGRATION_GUIDE.md)*
