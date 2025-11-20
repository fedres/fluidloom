# GoogleTest download (if not found)
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Define test executable
add_executable(fluidloom_tests
    main.cpp
    unit/test_mock_backend.cpp
    unit/test_opencl_backend.cpp
    unit/test_backend_factory.cpp
    unit/hilbert/test_hilbert_codec.cpp
    unit/fields/test_field_manager.cpp
    unit/hashmap/test_hash_table.cpp
    unit/hilbert/test_hilbert_opencl.cpp
    unit/halo/test_ghost_range.cpp
    unit/halo/test_halo_exchanger.cpp
    unit/parsing/test_parsing.cpp
    unit/parsing/test_fields_parser.cpp
    unit/parsing/test_lattices_parser.cpp
)

if(FL_ENABLE_OPENCL)
    list(APPEND TEST_SOURCES unit/test_opencl_backend.cpp)
endif()


target_link_libraries(fluidloom_tests 
    PRIVATE 
        fluidloom_core_objects
        fluidloom_parsing
        ${HALO_SOURCES}
        gtest_main
        MPI::MPI_CXX
)

target_include_directories(fluidloom_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(FL_ENABLE_OPENCL)
    target_link_libraries(fluidloom_tests PRIVATE OpenCL::OpenCL)
endif()
target_link_libraries(fluidloom_tests PRIVATE MPI::MPI_CXX)

# Make tests depend on kernel copy
add_dependencies(fluidloom_tests fluidloom_kernels)

# Add test discovery
include(GoogleTest)
gtest_discover_tests(fluidloom_tests)

# Memory testing targets
if(UNIX)
    find_program(VALGRIND_EXECUTABLE valgrind)
    if(VALGRIND_EXECUTABLE)
        add_custom_target(test_memcheck
            COMMAND ${VALGRIND_EXECUTABLE} --leak-check=full --error-exitcode=1 
                    ./fluidloom_tests
            DEPENDS fluidloom_tests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running tests with Valgrind"
        )
    endif()
endif()

# Thread sanitizer target
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_custom_target(test_tsan
        COMMAND ${CMAKE_COMMAND} -E env TSAN_OPTIONS=second_deadlock_stack=1 
                ./fluidloom_tests
        DEPENDS fluidloom_tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests with ThreadSanitizer"
    )
endif()

# Coverage target
if(FL_ENABLE_COVERAGE)
    target_compile_options(fluidloom_core_objects PRIVATE --coverage)
    target_link_libraries(fluidloom_tests PRIVATE --coverage)
    
    add_custom_target(coverage
        COMMAND ctest
        COMMAND gcovr -r ${CMAKE_SOURCE_DIR} --html --html-details -o coverage.html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report"
    )
endif()

# Subdirectories for additional tests
add_subdirectory(unit)
add_subdirectory(integration)
add_subdirectory(contract)
add_subdirectory(performance)
add_subdirectory(benchmark)
