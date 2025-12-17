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

message(STATUS "Tests configured: test_pwmanager")
