add_executable(test_halo_integration
    test_halo_integration.cpp
)

target_link_libraries(test_halo_integration
    GTest::gtest_main
    fluidloom_halo_objects
    fluidloom_transport_objects
    fluidloom_core_objects
)

target_include_directories(test_halo_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME HaloIntegrationTests COMMAND test_halo_integration)
