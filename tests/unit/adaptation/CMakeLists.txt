
set(TEST_SOURCES
    SplitEngineTest.cpp
    MergeEngineTest.cpp
    BalanceEnforcerTest.cpp
)

add_executable(adaptation_unit_tests ${TEST_SOURCES})

target_link_libraries(adaptation_unit_tests
    PRIVATE
    GTest::gtest_main
    fluidloom_adaptation
    fluidloom_core_objects
    OpenCL::OpenCL
)

# Add test
add_test(NAME AdaptationUnitTests COMMAND adaptation_unit_tests)
