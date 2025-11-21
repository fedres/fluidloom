cmake_minimum_required(VERSION 3.20)

# System tests require test harness
add_subdirectory(${CMAKE_SOURCE_DIR}/tools/test_harness ${CMAKE_BINARY_DIR}/tools/test_harness)

# Lid-driven cavity test
add_executable(lid_driven_cavity_test
    lid_driven_cavity/lid_driven_cavity_test.cpp
)

target_link_libraries(lid_driven_cavity_test
    PRIVATE
        fluidloom_test_harness
        gtest_main
)

target_include_directories(lid_driven_cavity_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

gtest_discover_tests(lid_driven_cavity_test)

# Taylor-Green vortex test
add_executable(taylor_green_test
    taylor_green_vortex/taylor_green_test.cpp
)

target_link_libraries(taylor_green_test
    PRIVATE
        fluidloom_test_harness
        gtest_main
)

target_include_directories(taylor_green_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

gtest_discover_tests(taylor_green_test)

# Wind tunnel test
add_executable(wind_tunnel_test
    wind_tunnel/wind_tunnel_test.cpp
)

target_link_libraries(wind_tunnel_test
    PRIVATE
        fluidloom_test_harness
        gtest_main
)

target_include_directories(wind_tunnel_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
)

gtest_discover_tests(wind_tunnel_test)

