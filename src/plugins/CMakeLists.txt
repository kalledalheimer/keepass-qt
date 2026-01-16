# KeePass Plugin System
add_library(keepass-plugins STATIC
    KpPluginInterface.h
    PluginManager.cpp
    PluginManager.h
)

target_link_libraries(keepass-plugins
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
    PRIVATE
        keepass-core
)

target_include_directories(keepass-plugins
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
