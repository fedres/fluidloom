# Unit tests for runtime components
add_executable(test_field_version_tracker test_field_version_tracker.cpp)
target_link_libraries(test_field_version_tracker
    GTest::gtest
    GTest::gtest_main
    fluidloom_runtime_objects
    fluidloom_core_objects
)
add_test(NAME FieldVersionTracker COMMAND test_field_version_tracker)

add_executable(test_hazard_analyzer test_hazard_analyzer.cpp)
target_link_libraries(test_hazard_analyzer
    GTest::gtest
    GTest::gtest_main
    fluidloom_runtime_objects
    fluidloom_core_objects
)
add_test(NAME HazardAnalyzer COMMAND test_hazard_analyzer)

add_executable(test_topological_scheduler test_topological_scheduler.cpp)
target_link_libraries(test_topological_scheduler
    GTest::gtest
    GTest::gtest_main
    fluidloom_runtime_objects
    fluidloom_core_objects
)
add_test(NAME TopologicalScheduler COMMAND test_topological_scheduler)
