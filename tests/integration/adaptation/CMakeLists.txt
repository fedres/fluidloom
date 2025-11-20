
set(ADAPTATION_INTEGRATION_TESTS
    AdaptationIntegrationTest.cpp
)

add_executable(adaptation_integration_tests ${ADAPTATION_INTEGRATION_TESTS})

target_link_libraries(adaptation_integration_tests PRIVATE
    fluidloom_adaptation
    fluidloom_core
    fluidloom_runtime_objects
    GTest::gtest_main
    OpenCL::OpenCL
)

target_include_directories(adaptation_integration_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME adaptation_integration_tests COMMAND adaptation_integration_tests)
