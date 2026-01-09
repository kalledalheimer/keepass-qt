# Qt-KeePass Tests

# Find Qt Test module
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

# Test: PwManager (core database functionality)
add_executable(test_pwmanager
    test_pwmanager.cpp
)

target_link_libraries(test_pwmanager
    PRIVATE
        keepass-core
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)

# Enable Qt Test features
set_target_properties(test_pwmanager PROPERTIES
    AUTOMOC ON
)

# Register test with CTest
add_test(NAME test_pwmanager COMMAND test_pwmanager)

# Set test working directory to project root (so tests/data paths work)
set_tests_properties(test_pwmanager PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Test: MFC Compatibility (round-trip validation)
add_executable(test_mfc_compatibility
    test_mfc_compatibility.cpp
)

target_link_libraries(test_mfc_compatibility
    PRIVATE
        keepass-core
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_mfc_compatibility PROPERTIES
    AUTOMOC ON
)

add_test(NAME test_mfc_compatibility COMMAND test_mfc_compatibility)

set_tests_properties(test_mfc_compatibility PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Test: Crypto Primitives (test vectors for AES, Twofish, SHA-256)
add_executable(test_crypto_primitives
    test_crypto_primitives.cpp
)

target_link_libraries(test_crypto_primitives
    PRIVATE
        keepass-core
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)

set_target_properties(test_crypto_primitives PROPERTIES
    AUTOMOC ON
)

add_test(NAME test_crypto_primitives COMMAND test_crypto_primitives)

set_tests_properties(test_crypto_primitives PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

message(STATUS "Tests configured: test_pwmanager, test_mfc_compatibility, test_crypto_primitives")

# Add validation tools subdirectory
add_subdirectory(tools)
