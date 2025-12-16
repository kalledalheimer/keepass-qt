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

    # Constants
    PwConstants.h
)

target_link_libraries(keepass-core
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
    PRIVATE
        OpenSSL::Crypto
)

target_include_directories(keepass-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Platform-specific sources (will be added later)
# if(WIN32)
#     target_sources(keepass-core PRIVATE
#         platform/WindowsPlatform.cpp
#         platform/WindowsPlatform.h
#     )
# elseif(APPLE)
#     target_sources(keepass-core PRIVATE
#         platform/MacPlatform.cpp
#         platform/MacPlatform.h
#     )
# else()
#     target_sources(keepass-core PRIVATE
#         platform/LinuxPlatform.cpp
#         platform/LinuxPlatform.h
#     )
# endif()

# Definitions for Windows compatibility
if(MSVC)
    target_compile_definitions(keepass-core PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()
