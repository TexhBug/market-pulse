
cmake_minimum_required(VERSION 3.16)

project(OrderBookVisualizer
  VERSION 1.0.0
  DESCRIPTION "A multithreaded order book visualizer"
  LANGUAGES CXX
)

# -----------------------------
# C++ Standard
# -----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------
# Compiler warnings
# -----------------------------
if (MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
endif()

# -----------------------------
# Static linking: Windows-only
# -----------------------------
# Static linking on Linux (glibc) is typically painful with libwebsockets/OpenSSL dependencies.
if (WIN32 AND NOT MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# -----------------------------
# Output directories
# -----------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# -----------------------------
# Threads
# -----------------------------
find_package(Threads REQUIRED)

# -----------------------------
# Option: WebSockets
# -----------------------------
option(ENABLE_WEBSOCKETS "Enable WebSocket server via libwebsockets" ON)

set(WEBSOCKETS_FOUND FALSE)
set(WEBSOCKETS_INCLUDE_DIR "")

if (ENABLE_WEBSOCKETS)
  if (WIN32)
    # ---- Windows/MSYS2 example paths (adjust if different) ----
    set(WEBSOCKETS_INCLUDE_DIR "C:/msys64/mingw64/include")
    set(WEBSOCKETS_LIBRARY     "C:/msys64/mingw64/lib/libwebsockets_static.a")
    set(OPENSSL_ROOT_DIR       "C:/msys64/mingw64")

    if (EXISTS "${WEBSOCKETS_LIBRARY}")
      set(WEBSOCKETS_FOUND TRUE)
      message(STATUS "Found libwebsockets (Windows): ${WEBSOCKETS_LIBRARY}")
    else()
      message(WARNING "libwebsockets not found on Windows. WebSocket server disabled.")
    endif()

  else()
    # ---- Linux: prefer pkg-config ----
    find_package(PkgConfig REQUIRED)

    # IMPORTED_TARGET gives you PkgConfig::LWS target with proper include dirs / link flags
    pkg_check_modules(LWS REQUIRED IMPORTED_TARGET libwebsockets)

    if (LWS_FOUND)
      set(WEBSOCKETS_FOUND TRUE)
      message(STATUS "Found libwebsockets (Linux) via pkg-config")
    else()
      message(WARNING "libwebsockets not found on Linux. WebSocket server disabled.")
    endif()
  endif()
endif()

# -----------------------------
# Sources / Headers
# -----------------------------
set(SOURCES
  src/main.cpp
  src/Order.cpp
  src/OrderBook.cpp
  src/MatchingEngine.cpp
  src/OrderQueue.cpp
  src/Visualizer.cpp
)

set(HEADERS
  include/Order.h
  include/OrderBook.h
  include/MatchingEngine.h
  include/OrderQueue.h
  include/Visualizer.h
  include/Common.h
)

if (WEBSOCKETS_FOUND)
  list(APPEND SOURCES src/WebSocketServer.cpp)
  list(APPEND HEADERS include/WebSocketServer.h)
endif()

# -----------------------------
# Target
# -----------------------------
add_executable(orderbook ${SOURCES} ${HEADERS})

target_include_directories(orderbook
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(orderbook
  PRIVATE
    Threads::Threads
)

# -----------------------------
# WebSockets linking
# -----------------------------
if (WEBSOCKETS_FOUND)
  target_compile_definitions(orderbook PRIVATE WEBSOCKET_ENABLED=1)

  if (WIN32)
    # Windows static libwebsockets + openssl + system libs
    target_include_directories(orderbook PRIVATE "${WEBSOCKETS_INCLUDE_DIR}")

    target_link_libraries(orderbook PRIVATE
      "${WEBSOCKETS_LIBRARY}"
      "${OPENSSL_ROOT_DIR}/lib/libssl.a"
      "${OPENSSL_ROOT_DIR}/lib/libcrypto.a"
      ws2_32
      crypt32
      bcrypt
    )
  else()
    # Linux: link the pkg-config imported target; it carries correct libs/backends.
    target_link_libraries(orderbook PRIVATE PkgConfig::LWS)

    # If your libwebsockets was built with OpenSSL (Ubuntu usually is), link it cleanly:
    find_package(OpenSSL REQUIRED)
    target_link_libraries(orderbook PRIVATE OpenSSL::SSL OpenSSL::Crypto)
  endif()
else()
  target_compile_definitions(orderbook PRIVATE WEBSOCKET_ENABLED=0)
endif()

# -----------------------------
# Testing (optional)
# -----------------------------
option(BUILD_TESTS "Build unit tests" OFF)
if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# -----------------------------
# Install (optional)
# -----------------------------
install(TARGETS orderbook DESTINATION bin)

# -----------------------------
# Build info
# -----------------------------
message(STATUS "")
message(STATUS "==============================")
message(STATUS " Order Book Visualizer v${PROJECT_VERSION}")
message(STATUS " C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS " Build Type : ${CMAKE_BUILD_TYPE}")
message(STATUS " WebSockets : ${WEBSOCKETS_FOUND}")
message(STATUS " Build Tests: ${BUILD_TESTS}")
message(STATUS "==============================")
message(STATUS "")
