include(FetchContent)
FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(benchmark_halo benchmark_halo.cpp)
target_link_libraries(benchmark_halo benchmark::benchmark fluidloom_halo_objects fluidloom_transport_objects fluidloom_core_objects)
target_include_directories(benchmark_halo PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)

# Transport benchmarks (requires MPI)
if(MPI_FOUND)
    add_executable(benchmark_transport
        benchmark_transport.cpp
    )
    
    target_link_libraries(benchmark_transport
        benchmark::benchmark
        fluidloom_transport_objects
        fluidloom_core_objects
        MPI::MPI_CXX
    )
    
    target_include_directories(benchmark_transport PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
endif()
