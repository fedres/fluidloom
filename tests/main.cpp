#include <gtest/gtest.h>
#include <cstdlib>

// Custom memory leak detector for Module 1
#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

int main(int argc, char** argv) {
    // Enable memory leak reporting on Windows
#ifdef _WIN32
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
