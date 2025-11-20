add_executable(test_two_gpu_exchange
    test_two_gpu_exchange.cpp
)

target_link_libraries(test_two_gpu_exchange
    GTest::gtest
    fluidloom_transport_objects
    fluidloom_core_objects
    MPI::MPI_CXX
)

target_include_directories(test_two_gpu_exchange PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# This test requires MPI with 2 ranks
add_test(
    NAME TwoGPUExchange
    COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
            $<TARGET_FILE:test_two_gpu_exchange> ${MPIEXEC_POSTFLAGS}
)

set_tests_properties(TwoGPUExchange PROPERTIES
    LABELS "integration;mpi;transport"
    TIMEOUT 60
)
