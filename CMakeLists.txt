cmake_minimum_required(VERSION 3.16)
project(Qt-KeePass VERSION 1.43.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt - support both Qt5 and Qt6
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} 5.15 REQUIRED COMPONENTS Core Gui Widgets Network PrintSupport LinguistTools)

# OpenSSL
find_package(OpenSSL 1.1.1 REQUIRED)

# Compiler flags for security and warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-fstack-protector-strong)
    add_compile_definitions(_FORTIFY_SOURCE=2)

    # Position Independent Executable
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(UNICODE _UNICODE)
endif()

# Core library
add_subdirectory(src/core)

# GUI application
add_subdirectory(src/gui)

# Auto-type system
add_subdirectory(src/autotype)

# Plugin system
add_subdirectory(src/plugins)

# Translations
set(TS_FILES
    translations/keepass_en.ts
    translations/keepass_de.ts
    translations/keepass_fr.ts
    translations/keepass_es.ts
)

# Generate .qm files from .ts files
if(Qt${QT_VERSION_MAJOR}LinguistTools_FOUND)
    if(QT_VERSION_MAJOR EQUAL 6)
        qt_add_translations(keepass-gui
            TS_FILES ${TS_FILES}
            QM_FILES_OUTPUT_VARIABLE QM_FILES
        )
    else()
        qt5_add_translation(QM_FILES ${TS_FILES})
    endif()
endif()

# Main executable
add_executable(keepass
    src/main.cpp
)

target_link_libraries(keepass
    PRIVATE
        keepass-core
        keepass-gui
        keepass-autotype
        keepass-plugins
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
)

# Set application properties
set_target_properties(keepass PROPERTIES
    OUTPUT_NAME "KeePass"
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Installation
install(TARGETS keepass
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# Tests
enable_testing()
add_subdirectory(tests)

# Print configuration summary
message(STATUS "")
message(STATUS "Qt-KeePass Configuration Summary")
message(STATUS "================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Qt Version: ${QT_VERSION}")
message(STATUS "OpenSSL Version: ${OPENSSL_VERSION}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
