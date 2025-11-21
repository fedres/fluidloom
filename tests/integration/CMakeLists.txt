# Integration tests
add_subdirectory(halo)
add_subdirectory(transport)
add_subdirectory(adaptation)

# Profiling Test
add_executable(profiling_test ProfilingTest.cpp)
target_link_libraries(profiling_test
    PRIVATE
    fluidloom_profiling
    fluidloom_core
    fluidloom_core_objects
    GTest::gtest_main
)
add_test(NAME ProfilingTest COMMAND profiling_test)

# VTK Test
add_executable(vtk_test VTKTest.cpp)
target_link_libraries(vtk_test
    PRIVATE
    fluidloom_io
    fluidloom_core
    fluidloom_core_objects
    fluidloom_profiling
    GTest::gtest_main
)
add_test(NAME VTKTest COMMAND vtk_test)
