# Docker & CMake Linux Deployment Issues

## Overview

When deploying the MarketPulse C++ backend to Render.com using Docker, we encountered multiple linking errors related to `libwebsockets` and its dependencies on Ubuntu 22.04.

---

## Issue 1: Package Name Mismatch

### Error
```
E: Unable to locate package libwebsockets19
```

### Cause
Ubuntu 22.04 uses `libwebsockets16`, not `libwebsockets19`.

### Fix
```dockerfile
# Wrong
RUN apt-get install -y libwebsockets19

# Correct
RUN apt-get install -y libwebsockets16
```

---

## Issue 2: Server Not Binding to External Interface

### Error
```
No open ports detected
```

### Cause
Server was binding to `localhost` instead of `0.0.0.0`, making it inaccessible from outside the container.

### Fix
In `src/WebSocketServer.cpp`:
```cpp
// Wrong
info.iface = nullptr;  // or "localhost"

// Correct
info.iface = "0.0.0.0";  // Bind to all interfaces
```

Also needed to read PORT from environment:
```cpp
const char* portEnv = std::getenv("PORT");
if (portEnv) {
    wsPort = std::atoi(portEnv);
}
```

---

## Issue 3: Static Linking on Linux

### Error
```
/usr/bin/ld: attempted static link of dynamic object `/usr/lib/x86_64-linux-gnu/libuv.so.1'
```

### Cause
CMakeLists.txt had `-static` flag applied globally, which works on Windows (MSYS2) but fails on Linux where `libwebsockets` dependencies are shared libraries.

### Fix
In `CMakeLists.txt`, apply `-static` only on Windows:
```cmake
# Wrong - applies to all platforms
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# Correct - Windows only
if (WIN32 AND NOT MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()
```

---

## Issue 4: Missing libev/libuv Dependencies

### Errors
```
undefined reference to `ev_signal_stop'
undefined reference to `ev_run'
undefined reference to `uv_stop'
undefined reference to `uv_is_active'
```

### Cause
Ubuntu's `libwebsockets` is compiled with support for multiple event loop backends:
- **libev** - lightweight event loop
- **libuv** - cross-platform async I/O (used by Node.js)

When linking against `libwebsockets`, these dependencies must also be linked.

### Fix

**Dockerfile - Build Stage:**
```dockerfile
RUN apt-get install -y \
    libwebsockets-dev \
    libev-dev \
    libuv1-dev \
    # ... other deps
```

**Dockerfile - Runtime Stage:**
```dockerfile
RUN apt-get install -y \
    libwebsockets16 \
    libev4 \
    libuv1 \
    # ... other deps
```

**CMakeLists.txt** - The key insight is that `pkg-config` with `IMPORTED_TARGET` handles dependencies automatically:
```cmake
# Linux linking
find_package(PkgConfig REQUIRED)
pkg_check_modules(LWS REQUIRED IMPORTED_TARGET libwebsockets)

target_link_libraries(orderbook PRIVATE 
    PkgConfig::LWS          # Handles libwebsockets + backends
    OpenSSL::SSL 
    OpenSSL::Crypto
)
```

---

## Issue 5: Cannot Find `-luv`

### Error
```
/usr/bin/ld: cannot find -luv: No such file or directory
```

### Cause
On Ubuntu, the libuv library is named `libuv.so.1` but the development symlink `libuv.so` may not exist, causing `-luv` to fail.

### Fix
Use `pkg-config` with `IMPORTED_TARGET` instead of manually specifying library names. This resolves the correct library paths automatically.

---

## Final Working Configuration

### Dockerfile
```dockerfile
# ---------- Build stage ----------
FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libwebsockets-dev \
    libssl-dev \
    libuv1-dev \
    libev-dev \
    zlib1g-dev \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt .
COPY src ./src
COPY include ./include
COPY tests ./tests

RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --parallel

# ---------- Runtime stage ----------
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libwebsockets16 \
    libssl3 \
    libuv1 \
    libev4 \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/bin/orderbook /app/orderbook

ENV PORT=10000
EXPOSE 10000

CMD ["/app/orderbook", "--headless", "--auto-start"]
```

### CMakeLists.txt (Linux Section)
```cmake
# Static linking: Windows-only
if (WIN32 AND NOT MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# ... later in WebSockets linking section ...

if (WIN32)
    # Windows static linking
    target_link_libraries(orderbook PRIVATE
      "${WEBSOCKETS_LIBRARY}"
      "${OPENSSL_ROOT_DIR}/lib/libssl.a"
      "${OPENSSL_ROOT_DIR}/lib/libcrypto.a"
      ws2_32 crypt32 bcrypt
    )
else()
    # Linux: use pkg-config IMPORTED_TARGET
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LWS REQUIRED IMPORTED_TARGET libwebsockets)
    
    target_link_libraries(orderbook PRIVATE PkgConfig::LWS)
    
    find_package(OpenSSL REQUIRED)
    target_link_libraries(orderbook PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()
```

---

## Key Lessons Learned

1. **Platform differences matter** - What works on Windows (MSYS2 static libs) doesn't work the same on Linux (shared libs with complex dependency chains).

2. **Use pkg-config on Linux** - It handles library paths, include directories, and transitive dependencies automatically.

3. **IMPORTED_TARGET is your friend** - `pkg_check_modules(LWS REQUIRED IMPORTED_TARGET libwebsockets)` creates a CMake target that carries all the right flags.

4. **Static linking on Linux is painful** - glibc, OpenSSL, and libwebsockets don't play well with static linking. Use shared libraries instead.

5. **Cloud deployments need 0.0.0.0** - Always bind to all interfaces, not localhost.

6. **Read PORT from environment** - Cloud platforms like Render set the PORT dynamically.

---

## Debugging Tips

### Check what libwebsockets was compiled with:
```bash
pkg-config --libs --static libwebsockets
```

### Check for missing symbols:
```bash
nm -u /usr/lib/x86_64-linux-gnu/libwebsockets.a | grep ev_
nm -u /usr/lib/x86_64-linux-gnu/libwebsockets.a | grep uv_
```

### Test Docker build locally:
```bash
docker build -t market-pulse .
docker run -p 10000:10000 market-pulse
```

### Check library dependencies of the binary:
```bash
ldd ./orderbook
```
