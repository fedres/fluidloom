add_executable(geometry_test
    GeometryVoxelizerTest.cpp
)

target_link_libraries(geometry_test
    PRIVATE
    fluidloom_core
    fluidloom_geometry
    GTest::gtest_main
)

add_test(NAME GeometryVoxelizerTest COMMAND geometry_test)
