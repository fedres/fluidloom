add_executable(test_transport_unit
    test_gpu_aware_buffer.cpp
    test_mpi_transport.cpp
)

# Add mock MPI definition for tests
target_compile_definitions(test_transport_unit PRIVATE FLUIDLOOM_MOCK_MPI FLUIDLOOM_MPI_ENABLED)

target_link_libraries(test_transport_unit
    GTest::gtest_main
    fluidloom_transport_objects
    fluidloom_core_objects
)

target_include_directories(test_transport_unit PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests/mock/mpi
)

add_test(NAME TransportUnitTests COMMAND test_transport_unit)
