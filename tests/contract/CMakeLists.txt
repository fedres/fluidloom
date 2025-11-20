# Contract tests
add_executable(test_contract
    module6_contract.cpp
)

target_link_libraries(test_contract
    GTest::gtest_main
    fluidloom_parsing
    fluidloom_core_objects
)

target_include_directories(test_contract PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME ContractTests COMMAND test_contract)
