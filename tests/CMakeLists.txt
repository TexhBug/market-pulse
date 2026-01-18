# ============================================================================
# ORDER BOOK VISUALIZER - Tests CMakeLists.txt
# ============================================================================
# Configuration for building unit tests using GoogleTest
# ============================================================================

# Fetch GoogleTest from GitHub
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# ============================================================================
# TEST EXECUTABLES
# ============================================================================

# Test source files
set(TEST_SOURCES
    test_order.cpp
    test_orderbook.cpp
    test_matching_engine.cpp
    test_order_queue.cpp
)

# Source files to test (excluding main.cpp)
set(SRC_UNDER_TEST
    ${CMAKE_SOURCE_DIR}/src/Order.cpp
    ${CMAKE_SOURCE_DIR}/src/OrderBook.cpp
    ${CMAKE_SOURCE_DIR}/src/MatchingEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/OrderQueue.cpp
)

# Create test executable
add_executable(orderbook_tests ${TEST_SOURCES} ${SRC_UNDER_TEST})

# Link GoogleTest and threading
target_link_libraries(orderbook_tests 
    PRIVATE 
        GTest::gtest_main
        Threads::Threads
)

# Include directories
target_include_directories(orderbook_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Enable GoogleTest discovery
include(GoogleTest)
gtest_discover_tests(orderbook_tests)
