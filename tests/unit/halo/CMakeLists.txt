# Halo unit tests

add_executable(test_halo_unit
    test_double_buffer.cpp
    test_interpolation.cpp
    test_pack_layout.cpp
    test_kernels.cpp
    test_exchange_manager.cpp
)

target_link_libraries(test_halo_unit
    GTest::gtest_main
    fluidloom_halo_objects
    fluidloom_transport_objects
    fluidloom_core_objects
)

target_include_directories(test_halo_unit PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME HaloUnitTests COMMAND test_halo_unit)
