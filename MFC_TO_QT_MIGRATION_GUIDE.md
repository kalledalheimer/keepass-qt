# MFC to Qt Migration Guide

**Purpose:** Comprehensive guide for migrating Windows/MFC applications to Qt framework
**Audience:** Developers and LLMs performing MFC-to-Qt migrations
**Last Updated:** 2026-01-06

This document provides reusable patterns, type mappings, coding conventions, and best practices for any MFC-to-Qt migration project.

---

## Table of Contents

1. [Type Mappings](#type-mappings)
2. [String Handling](#string-handling)
3. [Coding Style: KDE Guidelines](#coding-style-kde-guidelines)
4. [Memory Management](#memory-management)
5. [Platform Abstraction](#platform-abstraction)
6. [Common Patterns](#common-patterns)
7. [Development Guidelines](#development-guidelines)
8. [Security Best Practices](#security-best-practices)
9. [Git Workflow](#git-workflow)
10. [Migration Checklist](#migration-checklist)

---

## Type Mappings

### Integer Types

| Windows/MFC Type | Qt/C++ Type | Size | Notes |
|------------------|-------------|------|-------|
| `BYTE` | `quint8` or `uint8_t` | 1 byte | Prefer `quint8` for Qt consistency |
| `WORD` | `quint16` or `uint16_t` | 2 bytes | Prefer `quint16` for Qt consistency |
| `DWORD` | `quint32` or `uint32_t` | 4 bytes | Prefer `quint32` for Qt consistency |
| `QWORD` | `quint64` or `uint64_t` | 8 bytes | Prefer `quint64` for Qt consistency |
| `USHORT` | `quint16` or `uint16_t` | 2 bytes | Unsigned short |
| `UINT` | `unsigned int` | Platform-dependent | Avoid when size matters |
| `UINT8` | `quint8` | 1 byte | Qt provides consistent naming |
| `UINT16` | `quint16` | 2 bytes | |
| `UINT32` | `quint32` | 4 bytes | |
| `UINT64` | `quint64` | 8 bytes | |
| `INT8` | `qint8` | 1 byte | Signed 8-bit |
| `INT16` | `qint16` | 2 bytes | |
| `INT32` | `qint32` | 4 bytes | |
| `INT64` | `qint64` | 8 bytes | |
| `BOOL` | `bool` | 1 byte | Use C++ native `bool` |
| `LPARAM` | `qintptr` | Pointer-sized | For event parameters |
| `WPARAM` | `quintptr` | Pointer-sized | For event parameters |
| `SIZE_T` | `size_t` | Pointer-sized | Standard C++ type |

**Recommendation:** Use `quint8`, `quint16`, `quint32`, `quint64` for all fixed-size unsigned integers to maintain Qt consistency.

### String Types

| Windows/MFC Type | Qt Type | Notes |
|------------------|---------|-------|
| `TCHAR*` | `QString` | Always use `QString` internally |
| `TCHAR*` (file format) | `char*` + `QString::toUtf8()` | For serialization only |
| `LPCSTR` | `const char*` or `QString` | Prefer `QString` |
| `LPCTSTR` | `const QString&` | Pass by const reference |
| `LPWSTR` | `QString` | Wide string → QString |
| `CString` | `QString` | Direct replacement |
| `std::string` | `QString` or `QByteArray` | Depends on context |

**Key Principle:** Use `QString` for all internal string handling. Convert to/from UTF-8 only at I/O boundaries.

### Boolean Values

| Windows/MFC | Qt/C++ | Value |
|-------------|--------|-------|
| `TRUE` | `true` | 1 |
| `FALSE` | `false` | 0 |
| `BOOL` | `bool` | Native C++ type |

**Recommendation:** Always use lowercase `true`/`false` and `bool` type.

### Pointer and Handle Types

| Windows/MFC Type | Qt/C++ Type | Notes |
|------------------|-------------|-------|
| `LPVOID` | `void*` | Generic pointer |
| `HANDLE` | `quintptr` or platform-specific | Avoid if possible |
| `HWND` | Platform abstraction | Use Qt widgets instead |
| `HDC` | `QPainter` | Use Qt painting APIs |
| `HBITMAP` | `QPixmap` or `QImage` | Qt image types |

### MFC Classes to Qt Classes

| MFC Class | Qt Class | Notes |
|-----------|----------|-------|
| `CString` | `QString` | Full Unicode support |
| `CArray` | `QVector<T>` or `QList<T>` | Prefer `QVector` for contiguous data |
| `CList` | `QLinkedList<T>` | Linked list |
| `CMap` | `QMap<K,V>` or `QHash<K,V>` | Use `QHash` for better performance |
| `CFile` | `QFile` | File I/O |
| `CTime` | `QDateTime` | Date/time handling |
| `CPoint` | `QPoint` | 2D point |
| `CRect` | `QRect` | Rectangle |
| `CSize` | `QSize` | Size (width, height) |

---

## String Handling

### Internal String Storage

```cpp
// MFC (Wrong for Qt)
TCHAR* m_pszPassword;
CString m_strTitle;

// Qt (Correct)
QString m_password;
QString m_title;
```

### String Conversion for File I/O

```cpp
// Writing to binary file (KDB format uses UTF-8)
QString title = "My Entry";
QByteArray titleUtf8 = title.toUtf8();
file.write(titleUtf8.constData(), titleUtf8.length());

// Reading from binary file
QByteArray titleUtf8 = file.read(length);
QString title = QString::fromUtf8(titleUtf8);
```

### String Comparison

```cpp
// MFC
if (_tcscmp(str1, str2) == 0) { }

// Qt
if (str1 == str2) { }  // Natural comparison
if (str1.compare(str2, Qt::CaseInsensitive) == 0) { }  // Case-insensitive
```

---

## Coding Style: KDE Guidelines

### Naming Conventions

```cpp
// Variables and functions: camelCase
int maxConnections = 10;
QString userName;
void processData();

// Member variables: m_ prefix
class MyClass {
private:
    int m_count;
    QString m_name;
    QVector<int> m_items;
};

// Static variables: s_ prefix
static int s_instanceCount = 0;

// Constants: UPPER_SNAKE_CASE (C-style) or kCamelCase (modern)
const int MAX_BUFFER_SIZE = 1024;
constexpr int kMaxBufferSize = 1024;  // Modern C++
```

### Braces and Formatting

```cpp
// ALWAYS use braces, even for single-line bodies
if (condition) {
    doSomething();
}

// WRONG - MFC style (no braces)
if (condition)
    doSomething();  // ❌ Missing braces

// CORRECT - KDE style (always braces)
if (condition) {
    doSomething();  // ✅ Has braces
}

// else on same line as closing brace
if (condition) {
    doSomething();
} else {
    doSomethingElse();
}

// Functions: opening brace on new line
void myFunction()
{
    // function body
}

// Classes: opening brace on new line
class MyClass
{
public:
    MyClass();
    ~MyClass();
};
```

**Migration Note:** Legacy MFC code often omits braces for single-line statements. During migration, these should be added incrementally to avoid introducing bugs. Use automated tools or linters where possible.

### Indentation

```cpp
// 4 spaces (no tabs)
void processData()
{
    if (condition) {
        for (int i = 0; i < count; ++i) {
            if (items[i].isValid()) {
                processItem(items[i]);
            }
        }
    }
}
```

### Include Organization

```cpp
// 1. Own header (for .cpp files)
#include "myclass.h"

// 2. Qt headers (alphabetical, no module prefix)
#include <QDateTime>
#include <QFile>
#include <QString>

// 3. Standard C++ headers
#include <cstring>
#include <memory>

// 4. Other libraries
#include <openssl/evp.h>
```

### Include Guards

```cpp
// For file: passwordmanager.h
#ifndef PASSWORDMANAGER_H
#define PASSWORDMANAGER_H

// ... content ...

#endif // PASSWORDMANAGER_H
```

---

## Memory Management

### Smart Pointers vs Raw Pointers

```cpp
// MFC (Manual memory management)
MyClass* pObj = new MyClass();
// ... use pObj ...
delete pObj;

// Qt (Parent-child ownership for QObjects)
QWidget* widget = new QWidget(parent);  // parent will delete

// Qt (Smart pointers for non-QObjects)
std::unique_ptr<DataClass> data = std::make_unique<DataClass>();
std::shared_ptr<Config> config = std::make_shared<Config>();
```

### Qt Parent-Child Ownership

```cpp
// Parent automatically deletes children
QMainWindow* window = new QMainWindow();
QPushButton* button = new QPushButton("Click", window);
// No need to delete button - window's destructor handles it

delete window;  // Deletes window and all child widgets
```

### Arrays

```cpp
// MFC
BYTE* pBuffer = new BYTE[1024];
delete[] pBuffer;

// Qt (Prefer containers)
QByteArray buffer(1024, 0);  // Automatic memory management

// Or modern C++
std::vector<quint8> buffer(1024);
```

---

## Platform Abstraction

### File Paths

```cpp
// MFC (Windows-specific)
_T("C:\\Users\\Documents\\file.txt")

// Qt (Cross-platform)
QString filePath = QDir::homePath() + "/Documents/file.txt";
// or
QString filePath = QStandardPaths::writableLocation(
    QStandardPaths::DocumentsLocation) + "/file.txt";
```

### Byte Order (Endianness)

```cpp
// Use Qt's byte order functions
quint32 value = 0x12345678;
quint32 littleEndian = qToLittleEndian(value);
quint32 bigEndian = qToBigEndian(value);
```

### Settings Storage

```cpp
// MFC (Registry)
HKEY hKey;
RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\MyApp"), ...);

// Qt (Cross-platform)
QSettings settings("MyCompany", "MyApp");
settings.setValue("key", value);
QString value = settings.value("key").toString();
```

---

## Common Patterns

### Error Handling

```cpp
// MFC (Return codes)
int OpenFile(const TCHAR* path) {
    if (!path) return ERROR_INVALID_PARAM;
    // ...
    return ERROR_SUCCESS;
}

// Qt (Return bool + output parameters, or exceptions for critical errors)
bool openFile(const QString& path, QString* errorMessage = nullptr)
{
    if (path.isEmpty()) {
        if (errorMessage) {
            *errorMessage = "Invalid file path";
        }
        return false;
    }
    // ...
    return true;
}
```

### Iteration

```cpp
// MFC
for (int i = 0; i < array.GetSize(); ++i) {
    ProcessItem(array[i]);
}

// Qt (Range-based for loop - modern C++)
for (const auto& item : items) {
    processItem(item);
}

// Qt (Traditional for loop)
for (int i = 0; i < items.size(); ++i) {
    processItem(items[i]);
}
```

### NULL Checks

```cpp
// MFC
if (pObj != NULL) { }
ASSERT(pObj != NULL);

// Qt/Modern C++
if (pObj != nullptr) { }  // or simply: if (pObj)
Q_ASSERT(pObj != nullptr);
```

---

## Development Guidelines

### Coding Standards

**Naming Conventions:**
```cpp
// Classes: PascalCase
class PasswordManager {
    // ...
};

// Functions: camelCase
void processData();
bool openFile(const QString& path);

// Member variables: m_ prefix + camelCase
class MyClass {
private:
    int m_count;
    QString m_name;
    QVector<int> m_items;
};

// Static variables: s_ prefix + camelCase
static int s_instanceCount = 0;

// Constants: UPPER_SNAKE_CASE or kCamelCase
const int MAX_BUFFER_SIZE = 1024;
constexpr int kMaxBufferSize = 1024;  // Modern C++ style

// QString variables: s prefix (optional convention)
QString sTitle;
QString sPassword;

// NOT: Windows-style psz prefix
// char* pszTitle;  // ❌ Avoid for internal code
```

**Standard Compliance:**
- Use C++17 minimum (C++20 preferred for new projects)
- Enable all compiler warnings (`-Wall -Wextra`)
- Treat warnings as errors in CI builds

**Code Organization:**
```cpp
// Include order:
// 1. Own header (for .cpp files)
#include "myclass.h"

// 2. Qt headers (alphabetical)
#include <QDateTime>
#include <QFile>
#include <QString>

// 3. Standard C++ headers
#include <cstring>
#include <memory>

// 4. Third-party libraries
#include <openssl/evp.h>
```

### Type Usage Guidelines

**Use Qt types for all internal code:**

```cpp
// ✅ Correct - Qt types
quint8 byteValue;
quint16 wordValue;
quint32 dwordValue;
quint64 qwordValue;
bool isValid;

// ❌ Wrong - Windows types
BYTE byteValue;
WORD wordValue;
DWORD dwordValue;
QWORD qwordValue;
BOOL isValid;
```

**Exceptions where Windows types are acceptable:**
1. **File format compatibility** - When exact binary layout must match:
   ```cpp
   // KDB header must use exact sizes
   struct PW_DBHEADER {
       DWORD dwSignature1;  // Must be 4 bytes
       DWORD dwSignature2;
       // ... other fields matching spec
   };
   ```

2. **Third-party library interfaces** - When interfacing with external APIs:
   ```cpp
   // Crypto library expects specific types
   void AES_set_key(const UINT8* key, UINT bits);
   ```

3. **Temporary during migration** - Mark with TODO comments:
   ```cpp
   // TODO: Convert to quint32 after verifying compatibility
   DWORD legacyCounter;
   ```

**For all other code:**
- Member variables → Qt types
- Function parameters → Qt types
- Loop counters → Qt types or size_t
- Local variables → Qt types

### String Handling Guidelines

**Always use QString for internal string handling:**

```cpp
// ✅ Correct
QString title;
QString username;
QString filePath = QDir::homePath() + "/documents/file.txt";

// ❌ Wrong
char* pszTitle;
std::string title;
TCHAR* pszUsername;
```

**char* usage (exceptions only):**
- Binary file I/O (at serialization boundary only)
- Interfacing with C libraries
- Convert to QString immediately after reading

**String conversion patterns:**
```cpp
// Reading from binary file
QByteArray utf8Data = file.read(length);
QString text = QString::fromUtf8(utf8Data);

// Writing to binary file
QString text = "Hello";
QByteArray utf8Data = text.toUtf8();
file.write(utf8Data.constData(), utf8Data.length());
```

**Naming conventions:**
- `QString` variables: `sVariableName` or just descriptive name
- `char*` (file I/O only): `pszFieldName` (legacy boundary only)
- `QByteArray`: `vVariableName` or descriptive name

### Comments and Documentation

```cpp
// Good comments explain WHY, not WHAT
// ❌ Bad: The code itself is obvious
i++;  // Increment i

// ✅ Good: Explains the reason
i++;  // Skip null terminator in file format

// Document deviations from original code
// MFC version used registry; Qt uses QSettings for cross-platform support
QSettings settings("MyApp", "MyApp");

// Reference original file locations when porting
// Reference: MFC/KeePassLibCpp/PwManager.cpp:1234
void unlockPassword(PW_ENTRY* entry) {
    // ... implementation
}

// Document security-critical code
// XOR with session key for memory protection against dumps
for (quint32 i = 0; i < passwordLen; ++i) {
    password[i] ^= m_sessionKey[i % 32];
}
```

---

## Security Best Practices

### 1. Cryptography

**Never implement custom cryptography:**
```cpp
// ❌ Wrong - Custom crypto
void myEncryptFunction(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        data[i] ^= 0x42;  // Insecure!
    }
}

// ✅ Correct - Use established libraries
#include <openssl/evp.h>

void encryptData(const uint8_t* plaintext, size_t len,
                 uint8_t* ciphertext, const uint8_t* key, const uint8_t* iv)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);
    // ... proper encryption
    EVP_CIPHER_CTX_free(ctx);
}
```

**Use cryptographically secure random numbers:**
```cpp
// ❌ Wrong - Not cryptographically secure
int randomValue = qrand();

// ✅ Correct - Cryptographically secure
#include <openssl/rand.h>

uint8_t randomBytes[32];
RAND_bytes(randomBytes, sizeof(randomBytes));
```

### 2. Secure Memory Handling

**Clear sensitive data from memory:**
```cpp
// ❌ Wrong - May be optimized away
memset(password, 0, passwordLen);

// ✅ Correct - Guaranteed to execute
#include <openssl/crypto.h>
OPENSSL_cleanse(password, passwordLen);
```

**Lock sensitive memory to prevent swapping:**
```cpp
#include <sys/mman.h>  // Unix
// or
#include <windows.h>   // Windows

class SecureMemory {
public:
    SecureMemory(size_t size) : m_data(new uint8_t[size]), m_size(size) {
        // Lock memory to prevent swapping to disk
        #ifdef Q_OS_WIN
            VirtualLock(m_data, m_size);
        #else
            mlock(m_data, m_size);
        #endif
    }

    ~SecureMemory() {
        // Clear before unlocking
        OPENSSL_cleanse(m_data, m_size);

        #ifdef Q_OS_WIN
            VirtualUnlock(m_data, m_size);
        #else
            munlock(m_data, m_size);
        #endif

        delete[] m_data;
    }

    // Disable copy, enable move
    SecureMemory(const SecureMemory&) = delete;
    SecureMemory& operator=(const SecureMemory&) = delete;
    SecureMemory(SecureMemory&&) = default;
    SecureMemory& operator=(SecureMemory&&) = default;

    uint8_t* data() { return m_data; }
    size_t size() const { return m_size; }

private:
    uint8_t* m_data;
    size_t m_size;
};
```

### 3. Input Validation

**Always validate external input:**
```cpp
bool loadFile(const QString& filePath, QString* errorMsg = nullptr)
{
    // Validate path
    if (filePath.isEmpty()) {
        if (errorMsg) *errorMsg = "Empty file path";
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMsg) *errorMsg = file.errorString();
        return false;
    }

    // Validate file size
    qint64 fileSize = file.size();
    const qint64 maxSize = 100 * 1024 * 1024;  // 100 MB limit
    if (fileSize > maxSize) {
        if (errorMsg) *errorMsg = "File too large";
        return false;
    }

    // Read and validate content
    QByteArray data = file.readAll();
    if (data.isEmpty() && fileSize > 0) {
        if (errorMsg) *errorMsg = "Failed to read file";
        return false;
    }

    return true;
}
```

### 4. Bounds Checking

**Always check array bounds:**
```cpp
// ❌ Wrong - No bounds checking
void processArray(const int* arr, int index) {
    int value = arr[index];  // May overflow!
}

// ✅ Correct - With bounds checking
void processArray(const QVector<int>& arr, int index) {
    if (index < 0 || index >= arr.size()) {
        qWarning() << "Index out of bounds:" << index;
        return;
    }
    int value = arr[index];
}

// ✅ Better - Use at() for automatic bounds checking
void processArray(const QVector<int>& arr, int index) {
    try {
        int value = arr.at(index);  // Throws on out-of-bounds
    } catch (const std::out_of_range& e) {
        qWarning() << "Index out of bounds:" << e.what();
    }
}
```

### 5. Compiler Security Flags

**Enable security-hardening compiler flags:**
```cmake
# CMakeLists.txt
if(MSVC)
    # Windows: Stack protection, DEP, ASLR
    add_compile_options(/GS /guard:cf)
    add_link_options(/DYNAMICBASE /NXCOMPAT)
else()
    # Unix: Stack protection, fortify source, PIE
    add_compile_options(
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
        -fPIE
    )
    add_link_options(-pie)
endif()

# Enable all warnings
add_compile_options(-Wall -Wextra -Wpedantic)
```

---

## Git Workflow

### Branching Strategy

**Feature branches:**
```bash
# Create feature branch
git checkout -b feature/password-generator

# Work on feature...
git add .
git commit -m "Implement password generator dialog"

# Merge to main when complete
git checkout main
git merge feature/password-generator
```

**Phase branches for large projects:**
```bash
# Create phase branch
git checkout -b phase-1-core-library

# Work on multiple features within phase
# Merge phase branch when phase complete
```

### Commit Messages

**Good commit message format:**
```
Short summary (50 chars or less)

Longer explanation if needed. Explain WHY the change was made,
not WHAT was changed (the diff shows that).

- Reference original MFC files when porting
- Note any deviations from original implementation
- Link to issues: Fixes #123

MFC Reference: KeePassLibCpp/PwManager.cpp:1234-1456
```

**Examples:**
```bash
# ✅ Good commit messages
git commit -m "Add password strength indicator

Implements real-time password quality scoring based on entropy
calculation. Uses same algorithm as MFC version.

MFC Reference: WinGUI/NewGUI/PasswordDlg.cpp:567"

git commit -m "Replace CString with QString in PwEntry

Converts all string fields to use Qt QString instead of MFC
CString. Maintains UTF-8 encoding for file I/O compatibility.

MFC Reference: KeePassLibCpp/PwStructs.h:89-112"

# ❌ Bad commit messages
git commit -m "fix bug"
git commit -m "updates"
git commit -m "wip"
```

### Tagging Milestones

**Tag important milestones:**
```bash
# Phase completions
git tag -a phase-1-complete -m "Phase 1: Core library complete, KDB compatibility verified"
git push origin phase-1-complete

# Version releases
git tag -a v1.0.0 -m "Version 1.0.0: Full feature parity with MFC version"
git push origin v1.0.0
```

### Code Review Checklist

**Before committing:**
- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] No debug code or TODO comments (or documented with issue)
- [ ] Memory leaks checked (Valgrind/sanitizers)
- [ ] Security review for crypto/sensitive code
- [ ] References to original MFC files included
- [ ] Commit message is descriptive

---

## Migration Checklist

### Type System
- [ ] Replace `BYTE`, `WORD`, `DWORD` with `quint8`, `quint16`, `quint32`
- [ ] Replace `BOOL` with `bool`, `TRUE` with `true`, `FALSE` with `false`
- [ ] Replace `TCHAR*` with `QString`
- [ ] Replace `NULL` with `nullptr`
- [ ] Use `size_t` for sizes, `qsizetype` for Qt containers

### String Handling
- [ ] Convert all internal strings to `QString`
- [ ] Use UTF-8 encoding at file I/O boundaries
- [ ] Replace string manipulation functions:
  - `_tcscpy` → `QString::operator=`
  - `_tcslen` → `QString::length()`
  - `_tcscmp` → `QString::compare()` or `==`
  - `_tcscat` → `QString::append()` or `+`

### Memory Management
- [ ] Replace manual `new`/`delete` with smart pointers or Qt containers
- [ ] Use Qt parent-child ownership for QObjects
- [ ] Replace arrays with `QVector<T>` or `std::vector<T>`
- [ ] Use RAII for resource management

### Coding Style (KDE)
- [ ] Use 4-space indentation (no tabs)
- [ ] Always use braces for conditionals/loops
- [ ] Use `m_` prefix for member variables
- [ ] Use camelCase for variables and functions
- [ ] Opening braces on same line (except functions/classes)
- [ ] Organize includes properly
- [ ] Keep lines under 160 characters

### Platform Independence
- [ ] Replace Windows-specific file paths with `QDir` / `QStandardPaths`
- [ ] Replace Registry with `QSettings`
- [ ] Replace Windows error codes with Qt error handling
- [ ] Abstract platform-specific code behind interfaces

### Qt Best Practices
- [ ] Use Qt containers (`QVector`, `QList`, `QHash`) instead of STL where appropriate
- [ ] Use Qt's signals and slots for event handling
- [ ] Leverage Qt's Model/View architecture for data display
- [ ] Use `QFile`, `QDir` for file operations
- [ ] Use `QDateTime` for time handling

---

## Example: Complete Type Migration

### Before (MFC)

```cpp
class CPasswordEntry
{
public:
    BYTE uuid[16];
    DWORD uGroupId;
    TCHAR* pszTitle;
    TCHAR* pszPassword;
    DWORD uPasswordLen;
    PW_TIME tCreation;
    BOOL bExpires;
};

BOOL LoadEntry(const TCHAR* pszFile)
{
    FILE* fp = NULL;
    _tfopen_s(&fp, pszFile, _T("rb"));
    if (fp == NULL) return FALSE;

    // Read data...
    fclose(fp);
    return TRUE;
}
```

### After (Qt)

```cpp
class PasswordEntry
{
public:
    quint8 uuid[16];
    quint32 groupId;
    QString title;
    QString password;
    quint32 passwordLength;
    PwTime creationTime;
    bool expires;
};

bool loadEntry(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    // Read data using QDataStream or QFile::read()
    file.close();
    return true;
}
```

---

## Additional Resources

- **Qt Documentation:** https://doc.qt.io/
- **KDE Coding Style:** https://community.kde.org/Policies/Frameworks_Coding_Style
- **Qt API Design Principles:** https://wiki.qt.io/API_Design_Principles
- **Modern C++ Guidelines:** https://isocpp.github.io/CppCoreGuidelines/

---

## Notes for Future Migrations

1. **Start with data structures:** Replace Windows types first, then refactor logic
2. **Preserve file format compatibility:** If migrating a file-based application, maintain byte-level compatibility during I/O
3. **Test incrementally:** Migrate module by module, testing after each change
4. **Use static analysis:** Tools like clang-tidy can help identify Windows-specific code
5. **Document decisions:** Keep a migration log for architectural choices

---

## Conclusion

This guide provides comprehensive patterns for migrating MFC applications to Qt. Follow these guidelines to ensure:
- Type safety and cross-platform compatibility
- Consistent coding style
- Secure handling of sensitive data
- Maintainable, well-documented code

For project-specific decisions and requirements, maintain a separate `CLAUDE.md` file in your project root that references this guide.

---

*Last Updated: 2026-01-06*
*Applicable to: Any MFC-to-Qt migration project*
