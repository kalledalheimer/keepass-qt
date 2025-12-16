# KeePass GUI Library
add_library(keepass-gui STATIC
    # Placeholder (GUI will be implemented in Phase 3)
    GuiStub.cpp
)

target_link_libraries(keepass-gui
    PUBLIC
        keepass-core
        Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(keepass-gui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)
