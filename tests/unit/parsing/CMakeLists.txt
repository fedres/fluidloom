# Unit tests for parsing components
add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser
    GTest::gtest
    GTest::gtest_main
    fluidloom_parsing
    fluidloom_core_objects
)
add_test(NAME ParserTests COMMAND test_parser)

# Semantic analyzer test
add_executable(test_semantic
    test_semantic.cpp
)

target_link_libraries(test_semantic
    fluidloom_parsing
    fluidloom_core_objects
    gtest_main
)

add_test(NAME SemanticTests COMMAND test_semantic)
