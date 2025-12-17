# Active Tasks

Last updated: 2025-12-17

## 🔴 High Priority - Phase 1 Completion (15% remaining)

- [ ] Implement `saveDatabase()` method in PwManager (#1) - **SECONDARY BLOCKER**
  - Serialize groups and entries to field format
  - Encrypt database content
  - Write KDB v1.x file with correct header
  - Reference: `MFC/MFC-KeePass/KeePassLibCpp/Details/PwFileImpl.cpp:373-600+`

- [ ] Implement CRUD operations (#2) - **REQUIRED FOR TESTING**
  - `addGroup()` - Add group to database (~50 lines)
  - `addEntry()` - Add entry to database (~80 lines)
  - `deleteEntry()`, `deleteGroupById()` - Remove operations
  - `setGroup()`, `setEntry()` - Update operations

- [ ] Create compatibility test suite (#3)
  - Generate reference KDB files using MFC KeePass
  - Test: Open MFC-generated KDB files
  - Test: Save KDB files readable by MFC version
  - Test: Round-trip (MFC → Qt → MFC → compare)

- [ ] Validate Phase 1 completion criteria
  - Byte-level header comparison
  - Field-by-field entry comparison
  - Verify encryption/decryption correctness
  - Test with different algorithms (AES, Twofish)
  - Test with different key rounds (1K to 10M)

## 🟡 Medium Priority - Phase 1 Polish

- [ ] Implement password lock/unlock for in-memory encryption (#4)
  - `lockEntryPassword()` - XOR password with session key
  - `unlockEntryPassword()` - Decrypt for use

- [ ] Create unit tests for crypto primitives
  - Crypto primitives (AES, Twofish, SHA-256)
  - Key transformation with known vectors
  - Time compression/decompression
  - Memory protection functions

## 🟢 Low Priority - Future Phases

- [ ] Implement search operations
  - `find()`, `findEx()`

- [ ] Add Doxygen documentation to all classes
  - Document deviations from MFC version
  - Explain crypto/security-critical code

## ✅ Recently Completed (2025-12-17)

### Session 2: Code Modernization & Testing
- [x] Created comprehensive MFC to Qt migration guide document
- [x] Refactored type system to use Qt types (quint8, quint16, quint32)
- [x] Modernized constants to constexpr with namespaces
- [x] Added enum classes (PwAlgorithm, PwError)
- [x] Enhanced documentation for PwManager class
- [x] Created unit test infrastructure with Qt Test framework
- [x] Implemented 10 comprehensive unit tests for PwManager
- [x] Set up CTest integration for automated testing
- [x] Created GitHub milestones for all 7 project phases
- [x] Synced tasks with GitHub Issues

### Session 1: Core Implementation (2025-12-16)
- [x] Implemented `openDatabase()` method in PwManager (~400 lines) ✅
  - Read and parse KDB v1.x file format (124-byte header)
  - Decrypt database content (AES-256-CBC or Twofish)
  - Parse groups and entries field-by-field
  - Handle meta-streams (stub implementation)
  - KDBX format detection and rejection
- [x] Implemented helper methods for KDB parsing
  - `readGroupField()` - Parse group field types
  - `readEntryField()` - Parse entry field types
  - `readExtData()` - Parse extended data
- [x] Created TwofishClass wrapper for Qt
- [x] Fixed all compilation errors
- [x] Successfully built entire project
- [x] Project infrastructure setup (CMake, Qt5/Qt6, OpenSSL)
- [x] All KDB v1.x data structures (PW_TIME, PW_DBHEADER, PW_GROUP, PW_ENTRY)
- [x] Cryptography layer (Rijndael, Twofish, SHA-256)
- [x] Cross-platform memory protection (mlock, OPENSSL_cleanse)
- [x] OpenSSL-based key transformation with parallel processing
- [x] PwManager core framework and session key encryption
- [x] Utility classes (Random, MemUtil, PwUtil)
- [x] Test application with Phase 1 status dialog

---

## 📊 Phase 1 Status: 90% Complete

**Critical Path:**
1. ✅ openDatabase() - DONE
2. 🔄 CRUD operations (addGroup, addEntry) - IN PROGRESS (tests written)
3. ⏳ saveDatabase() - NEXT
4. ⏳ Compatibility testing - FINAL VALIDATION

**Current Test Results:**
- ✓ 7/12 unit tests passing
- ✗ 5/12 tests failing (expected - awaiting implementations)
  - testAddGroup, testAddEntry → Need CRUD methods
  - testSaveAndOpenEmptyDatabase → Need saveDatabase()
  - testSaveAndOpenDatabaseWithData → Need saveDatabase() + CRUD
  - testPasswordEncryption → Need lockEntryPassword()

**Estimated Remaining:** 2-3 days of focused development

**Milestone Goal:** Can open/save KDB files with 100% MFC compatibility

---

## 🎯 GitHub Integration

- **Milestones:** 7 phases created with due dates
- **Open Issues:** 4 high/medium priority tasks
- **Labels:** priority:high, priority:medium, priority:low, core, testing, documentation

View all issues: https://github.com/kalledalheimer/keepass-qt/issues

---

*This file is automatically managed by Claude Code. Manual edits are welcome.*
