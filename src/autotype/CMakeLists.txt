# Auto-Type System
# Platform-specific keyboard simulation for auto-typing passwords

set(AUTOTYPE_SOURCES
    AutoTypeSequence.cpp
    AutoTypeSequence.h
    platform/AutoTypePlatform.cpp
    platform/AutoTypePlatform.h
)

# Platform-specific sources
if(APPLE)
    list(APPEND AUTOTYPE_SOURCES
        platform/AutoTypeMac.cpp
        platform/AutoTypeMac.h
    )
endif()

# Create auto-type library
add_library(keepass-autotype STATIC ${AUTOTYPE_SOURCES})

target_link_libraries(keepass-autotype
    PUBLIC
        keepass-core
        Qt${QT_VERSION_MAJOR}::Core
)

# macOS-specific frameworks
if(APPLE)
    target_link_libraries(keepass-autotype
        PUBLIC
            "-framework ApplicationServices"
            "-framework Carbon"
    )
endif()

target_include_directories(keepass-autotype
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
