# Qt-KeePass Documentation Guide

This guide describes the documentation standards for the Qt-KeePass project using Doxygen.

## Quick Start

### Generating Documentation

```bash
# From project root
doxygen Doxyfile

# Documentation output:
docs/api/html/index.html
```

### Viewing Documentation

```bash
# Open in browser
open docs/api/html/index.html  # macOS
xdg-open docs/api/html/index.html  # Linux
start docs/api/html/index.html  # Windows
```

## Documentation Standards

### File Headers

Every header file should start with a file documentation block:

```cpp
/**
 * @file ClassName.h
 * @brief One-line description of the file's purpose
 *
 * Detailed description explaining what this file contains,
 * its role in the architecture, and any important notes.
 *
 * @see RelatedClass for related functionality
 * @see MFC OriginalFile.cpp for original implementation reference
 */
```

### Class Documentation

Document classes with comprehensive descriptions:

```cpp
/**
 * @brief One-line summary of what the class does
 *
 * Detailed description explaining:
 * - The class's purpose and responsibilities
 * - How it fits into the architecture
 * - Key design decisions
 *
 * @par Usage Example:
 * @code
 * MyClass obj;
 * obj.doSomething();
 * @endcode
 *
 * @par Thread Safety:
 * This class is not thread-safe. [if applicable]
 *
 * @note Important implementation notes
 * @warning Critical warnings for API users
 * @see RelatedClass for related functionality
 */
class MyClass
{
    // ...
};
```

### Method Documentation

Document all public methods:

```cpp
/**
 * @brief One-line description of what the method does
 *
 * Detailed description if needed, explaining:
 * - What the method does
 * - Side effects
 * - Special behavior
 *
 * @param paramName Description of the parameter
 * @param anotherParam Description of another parameter
 * @return Description of return value (if not void)
 * @retval true Description of when true is returned
 * @retval false Description of when false is returned
 *
 * @throws std::exception Description of when exception is thrown
 * @pre Preconditions that must be met
 * @post Postconditions after method executes
 *
 * @note Additional notes
 * @warning Warnings about usage
 * @see relatedMethod() for related functionality
 */
bool myMethod(int paramName, const QString& anotherParam);
```

### Enum Documentation

```cpp
/**
 * @brief Description of the enum
 *
 * Detailed explanation of what the enum represents.
 */
enum class MyEnum {
    Value1,     ///< Brief description of Value1
    Value2,     ///< Brief description of Value2
    Value3      ///< Brief description of Value3
};
```

### Member Variable Documentation

```cpp
private:
    int m_count;              ///< Number of items processed
    QString m_name;           ///< Name of the object
    QList<Item*> m_items;     ///< List of child items
```

## Doxygen Tags Reference

### Common Tags

| Tag | Purpose | Example |
|-----|---------|---------|
| `@brief` | One-line summary | `@brief Opens a database file` |
| `@param` | Parameter description | `@param filePath Path to database` |
| `@return` | Return value | `@return true on success` |
| `@retval` | Specific return value | `@retval PWE_SUCCESS Success` |
| `@note` | Additional information | `@note Thread-safe` |
| `@warning` | Important warning | `@warning Modifies state` |
| `@see` | Cross-reference | `@see saveDatabase()` |
| `@throws` | Exception | `@throws IOException On file error` |
| `@deprecated` | Deprecated API | `@deprecated Use newMethod()` |

### Code Examples

Use `@code` and `@endcode` for code examples:

```cpp
/**
 * @par Example:
 * @code
 * PwManager mgr;
 * mgr.newDatabase();
 * mgr.setMasterKey("password", false, "", true, "");
 * @endcode
 */
```

### Lists

Bullet lists:

```cpp
/**
 * This method:
 * - Does thing A
 * - Does thing B
 * - Does thing C
 */
```

Numbered lists:

```cpp
/**
 * Algorithm steps:
 * -# First step
 * -# Second step
 * -# Third step
 */
```

### Cross-References

- Link to classes: `MyClass`
- Link to methods: `myMethod()`
- Link to members: `#m_member`
- Link to files: `@file MyFile.h`

## Priority Guidelines

### High Priority (Document First)

1. **Public APIs** - All public methods and classes
2. **Core Classes**:
   - `PwManager` - Database management
   - `UpdateChecker` - Network operations
   - `PasswordGenerator` - Password generation
3. **Crypto Classes** - Security-critical code
4. **Deviations from MFC** - Where Qt port differs

### Medium Priority

1. GUI classes (dialogs, widgets)
2. Utility classes
3. Import/Export functionality

### Low Priority

1. Private methods (unless complex)
2. Trivial getters/setters
3. Internal implementation details

## Style Conventions

### DO:
- ✅ Write clear, concise descriptions
- ✅ Use proper grammar and punctuation
- ✅ Provide usage examples for complex APIs
- ✅ Document parameters and return values
- ✅ Explain **why**, not just **what**
- ✅ Cross-reference related functionality
- ✅ Note deviations from MFC version

### DON'T:
- ❌ Repeat obvious information (`/// Gets the name` for `getName()`)
- ❌ Use vague descriptions (`/// Does stuff`)
- ❌ Document every trivial method
- ❌ Copy/paste without customizing
- ❌ Leave TODOs in production docs

## Example: Complete Class Documentation

See `src/core/UpdateChecker.h` for a comprehensive example of:
- File header documentation
- Class documentation with usage examples
- Method documentation with parameters and return values
- Enum and struct documentation
- Signal documentation

## Tools and Configuration

### Doxygen Configuration

Main configuration file: `Doxyfile` (project root)

Key settings:
- **INPUT**: Directories to scan (`src/core`, `src/gui`, etc.)
- **EXCLUDE**: Files to skip (generated code, external libraries)
- **EXTRACT_ALL**: Currently `YES` (documents everything)
- **WARNINGS**: Enabled to catch undocumented APIs

### Doxygen Warnings

Enable warnings for undocumented code:

```bash
# Check for documentation coverage
doxygen Doxyfile 2>&1 | grep "warning:"
```

## Integration with Build System

### CMake Target (Future)

```cmake
# Add custom target for documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()
```

### Building Documentation

```bash
# With CMake target (future)
cmake --build . --target doc

# Manual
doxygen Doxyfile
```

## Resources

- Doxygen Manual: https://www.doxygen.nl/manual/
- Doxygen Commands: https://www.doxygen.nl/manual/commands.html
- Qt Documentation Style: https://doc.qt.io/qt-6/03-qdoc-commands-markup.html

## Contributing

When adding new classes or modifying APIs:

1. Document the class/method **before** committing
2. Follow the standards in this guide
3. Use `UpdateChecker.h` as a reference example
4. Run Doxygen to verify documentation renders correctly
5. Fix any Doxygen warnings

---

*Last updated: 2026-01-19*
