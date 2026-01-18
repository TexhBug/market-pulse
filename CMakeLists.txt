# ============================================================================
# ORDER BOOK VISUALIZER - Root CMakeLists.txt
# ============================================================================
# This is the master build configuration for our project.
# It tells CMake how to build everything.
# ============================================================================

# Minimum CMake version required (3.16 has good C++17 support)
cmake_minimum_required(VERSION 3.16)

# Project name and version
project(OrderBookVisualizer 
    VERSION 1.0.0
    DESCRIPTION "A multithreaded order book visualizer"
    LANGUAGES CXX
)

# ============================================================================
# C++ STANDARD SETTINGS
# ============================================================================
# We're using C++17 for modern features like:
# - std::optional, std::variant
# - Structured bindings
# - if constexpr
# - Parallel algorithms

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# COMPILER FLAGS
# ============================================================================
# These flags help catch bugs and improve code quality

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang specific flags
    add_compile_options(
        -Wall           # Enable most warnings
        -Wextra         # Enable extra warnings
        -Wpedantic      # Strict ISO C++ compliance
        -Werror=return-type  # Error if function doesn't return a value
    )
    
    # ========================================================================
    # STATIC LINKING - No more DLL issues!
    # ========================================================================
    # This embeds the runtime libraries INTO the executable
    # So you don't need MinGW DLLs in PATH to run it!
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    
elseif(MSVC)
    # Microsoft Visual C++ flags
    add_compile_options(
        /W4             # High warning level
        /permissive-    # Strict standard compliance
    )
endif()

# ============================================================================
# OUTPUT DIRECTORIES
# ============================================================================
# Put all compiled files in organized folders

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# INCLUDE DIRECTORIES
# ============================================================================
# Tell the compiler where to find our header files

include_directories(${CMAKE_SOURCE_DIR}/include)

# ============================================================================
# THREADING SUPPORT
# ============================================================================
# Required for std::thread, std::mutex, etc.

find_package(Threads REQUIRED)

# ============================================================================
# WEBSOCKET SUPPORT (libwebsockets)
# ============================================================================
# Required for frontend communication

if(WIN32)
    # Find libwebsockets - installed via MSYS2
    # Headers: C:/msys64/mingw64/include/libwebsockets.h
    # Library: C:/msys64/mingw64/lib/libwebsockets_static.a
    set(WEBSOCKETS_INCLUDE_DIR "C:/msys64/mingw64/include")
    set(WEBSOCKETS_LIBRARY "C:/msys64/mingw64/lib/libwebsockets_static.a")
    set(OPENSSL_ROOT "C:/msys64/mingw64")

    # Check if libwebsockets is available
    if(EXISTS ${WEBSOCKETS_LIBRARY})
        set(WEBSOCKETS_FOUND TRUE)
        message(STATUS "Found libwebsockets: ${WEBSOCKETS_LIBRARY}")
    else()
        set(WEBSOCKETS_FOUND FALSE)
        message(WARNING "libwebsockets not found - WebSocket server will be disabled")
    endif()
else()
    # Linux - use system libwebsockets
    find_path(WEBSOCKETS_INCLUDE_DIR libwebsockets.h)
    find_library(WEBSOCKETS_LIBRARY NAMES websockets)
    
    if(WEBSOCKETS_INCLUDE_DIR AND WEBSOCKETS_LIBRARY)
        set(WEBSOCKETS_FOUND TRUE)
        message(STATUS "Found libwebsockets: ${WEBSOCKETS_LIBRARY}")
    else()
        # Try pkg-config
        find_package(PkgConfig)
        if(PkgConfig_FOUND)
            pkg_check_modules(WEBSOCKETS libwebsockets)
            if(WEBSOCKETS_FOUND)
                message(STATUS "Found libwebsockets via pkg-config")
            endif()
        endif()
        
        if(NOT WEBSOCKETS_FOUND)
            set(WEBSOCKETS_FOUND FALSE)
            message(WARNING "libwebsockets not found - WebSocket server will be disabled")
        endif()
    endif()
endif()

# ============================================================================
# SOURCE FILES
# ============================================================================
# List all our source files

set(SOURCES
    src/main.cpp
    src/Order.cpp
    src/OrderBook.cpp
    src/MatchingEngine.cpp
    src/OrderQueue.cpp
    src/Visualizer.cpp
)

# Add WebSocket source if available
if(WEBSOCKETS_FOUND)
    list(APPEND SOURCES src/WebSocketServer.cpp)
endif()

set(HEADERS
    include/Order.h
    include/OrderBook.h
    include/MatchingEngine.h
    include/OrderQueue.h
    include/Visualizer.h
    include/Common.h
)

if(WEBSOCKETS_FOUND)
    list(APPEND HEADERS include/WebSocketServer.h)
endif()

# ============================================================================
# MAIN EXECUTABLE
# ============================================================================
# Create the main program

add_executable(orderbook ${SOURCES} ${HEADERS})

# Link threading library
target_link_libraries(orderbook PRIVATE Threads::Threads)

# Link WebSocket library if available
if(WEBSOCKETS_FOUND)
    target_include_directories(orderbook PRIVATE ${WEBSOCKETS_INCLUDE_DIR})
    
    if(WIN32)
        # Windows-specific linking
        target_link_libraries(orderbook PRIVATE 
            ${WEBSOCKETS_LIBRARY}
            ${OPENSSL_ROOT}/lib/libssl.a
            ${OPENSSL_ROOT}/lib/libcrypto.a
            ws2_32      # Windows Sockets
            crypt32     # Windows Crypto
            bcrypt      # Windows Crypto
        )
    else()
        # Linux/Unix linking - use pkg-config for proper flags
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(LWS REQUIRED libwebsockets)
        pkg_check_modules(OPENSSL REQUIRED openssl)
        
        target_include_directories(orderbook PRIVATE ${LWS_INCLUDE_DIRS})
        target_link_libraries(orderbook PRIVATE 
            ${LWS_LIBRARIES} 
            ${OPENSSL_LIBRARIES}
            ev      # libwebsockets dependency
            uv      # libwebsockets dependency
        )
    endif()
    
    target_compile_definitions(orderbook PRIVATE WEBSOCKET_ENABLED=1)
endif()

# ============================================================================
# TESTING (Optional - enabled with -DBUILD_TESTS=ON)
# ============================================================================

option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# INSTALLATION (Optional)
# ============================================================================

install(TARGETS orderbook DESTINATION bin)

# ============================================================================
# PRINT BUILD INFO
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "  Order Book Visualizer v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
message(STATUS "========================================")
message(STATUS "")
