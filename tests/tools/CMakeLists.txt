# Validation tools

add_executable(generate_validation_suite
    generate_validation_suite.cpp
)

target_link_libraries(generate_validation_suite
    keepass-core
    Qt::Core
)

# Set output directory
set_target_properties(generate_validation_suite PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/tools"
)
