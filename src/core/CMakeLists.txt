# KeePass Core Library
add_library(keepass-core STATIC
    # System definitions
    SysDefEx.h

    # Data structures
    PwStructs.h

    # Core manager
    PwManager.cpp
    PwManager.h

    # Cryptography
    crypto/Rijndael.cpp
    crypto/Rijndael.h
    crypto/Twofish.cpp
    crypto/Twofish.h
    crypto/TwofishClass.cpp
    crypto/TwofishClass.h
    crypto/KeyTransform.cpp
    crypto/KeyTransform.h
    crypto/MemoryProtection.cpp
    crypto/MemoryProtection.h

    # SHA2
    crypto/SHA256.cpp
    crypto/SHA256.h
    crypto/SHA2/SHA2.cpp
    crypto/SHA2/SHA2.h
    crypto/SHA2/EDefs.h
    crypto/SHA2/TDefs.h

    # Utilities
    util/Random.cpp
    util/Random.h
    util/MemUtil.cpp
    util/MemUtil.h
    util/PwUtil.cpp
    util/PwUtil.h
    util/CsvUtil.cpp
    util/CsvUtil.h

    # Import/Export
    io/PwExport.cpp
    io/PwExport.h
    io/PwImport.cpp
    io/PwImport.h

    # Password Generator
    PasswordGenerator.cpp
    PasswordGenerator.h

    # SPR Engine (String Placeholder Replacement)
    SprEngine.cpp
    SprEngine.h

    # Constants
    PwConstants.h

    # Platform layer
    platform/PwSettings.cpp
    platform/PwSettings.h
    platform/MemoryProtection.h

)

target_link_libraries(keepass-core
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
    PRIVATE
        OpenSSL::Crypto
)

target_include_directories(keepass-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Platform-specific sources
if(WIN32)
    target_sources(keepass-core PRIVATE
        platform/MemoryProtection_win.cpp
    )
else()
    # Unix (macOS/Linux)
    target_sources(keepass-core PRIVATE
        platform/MemoryProtection_unix.cpp
    )
endif()

# Definitions for Windows compatibility
if(MSVC)
    target_compile_definitions(keepass-core PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()
