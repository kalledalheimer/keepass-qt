# KeePass Auto-Type System
add_library(keepass-autotype STATIC
    # Placeholder (auto-type will be implemented in Phase 5)
    AutoTypeStub.cpp
)

target_link_libraries(keepass-autotype
    PUBLIC
        keepass-core
        Qt${QT_VERSION_MAJOR}::Core
)

target_include_directories(keepass-autotype
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Platform-specific auto-type implementations
if(WIN32)
    target_sources(keepass-autotype PRIVATE
        # platform/AutoTypeWindows.cpp
        # platform/AutoTypeWindows.h
    )
elseif(APPLE)
    target_sources(keepass-autotype PRIVATE
        # platform/AutoTypeMac.cpp
        # platform/AutoTypeMac.h
    )
    # macOS requires CoreGraphics framework
    find_library(COREGRAPHICS_LIBRARY CoreGraphics REQUIRED)
    target_link_libraries(keepass-autotype PRIVATE ${COREGRAPHICS_LIBRARY})
else()
    target_sources(keepass-autotype PRIVATE
        # platform/AutoTypeX11.cpp
        # platform/AutoTypeX11.h
    )
    # Linux requires X11 and XTest
    find_package(X11 REQUIRED)
    target_link_libraries(keepass-autotype PRIVATE ${X11_LIBRARIES} ${X11_XTest_LIB})
endif()
