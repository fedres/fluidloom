# Performance tests
add_executable(test_parser_perf
    test_parser_perf.cpp
)

target_link_libraries(test_parser_perf
    fluidloom_parsing
    fluidloom_core_objects
)

target_include_directories(test_parser_perf PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
