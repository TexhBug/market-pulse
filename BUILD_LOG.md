# MarketPulse - Build Command Log

> **Project**: MarketPulse (Real-Time Order Book Visualizer)  
> **Purpose**: Complete record of commands run during development  
> **Usage**: Reference for rebuilding or troubleshooting

This document contains the actual commands executed during development.
Use this as a command reference when rebuilding from scratch.

---

## Table of Contents

1. [Environment Setup Commands](#1-environment-setup-commands)
2. [MSYS2 & MinGW Installation](#2-msys2--mingw-installation)
3. [CMake Build Commands](#3-cmake-build-commands)
4. [Node.js Frontend Commands](#4-nodejs-frontend-commands)
5. [Server Startup Commands](#5-server-startup-commands)
6. [Common Error Resolutions](#6-common-error-resolutions)
7. [Quick Reference Commands](#7-quick-reference-commands)

---

## 1. Environment Setup Commands

### Install MSYS2
```powershell
# Option 1: Download installer from https://www.msys2.org/
# Option 2: Use winget (recommended)
winget install -e --id MSYS2.MSYS2 --accept-package-agreements --accept-source-agreements
```

### Install CMake
```powershell
winget install -e --id Kitware.CMake --accept-package-agreements --accept-source-agreements
```

### Install Node.js
```powershell
winget install -e --id OpenJS.NodeJS.LTS --accept-package-agreements --accept-source-agreements
```

### Add MinGW to PATH (REQUIRED every PowerShell session)
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
```
**Why?** PowerShell doesn't inherit MSYS2's PATH. Without this, `g++` and `cmake` won't be found.

### Verify Tools
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
g++ --version      # GCC 15.1.0 or higher
cmake --version    # CMake 4.0+ 
node --version     # Node.js 18+
```

---

## 2. MSYS2 & MinGW Installation

### Open MSYS2 Terminal
Run **"MSYS2 MINGW64"** from Start Menu (NOT "MSYS2 MSYS").

### Update Package Database
```bash
pacman -Syu
# If terminal closes, reopen and run again:
pacman -Su
```

### Install C++ Toolchain
```bash
pacman -S mingw-w64-x86_64-toolchain
# Press Enter to accept all packages (~1GB download)
```
**What's installed:**
- `g++.exe` - C++ compiler
- `gcc.exe` - C compiler  
- `gdb.exe` - Debugger
- `make.exe` - Build tool

### Install CMake
```bash
pacman -S mingw-w64-x86_64-cmake
```

### Install libwebsockets
```bash
pacman -S mingw-w64-x86_64-libwebsockets
```
**What's installed:**
- `libwebsockets.dll` - WebSocket library
- Header files in `/mingw64/include/libwebsockets.h`

### Verify Installation (in MSYS2 terminal)
```bash
g++ --version
# g++ (Rev1, Built by MSYS2 project) 15.1.0

cmake --version
# cmake version 4.0.1

pkg-config --libs libwebsockets
# -lwebsockets
```

---

## 3. CMake Build Commands

### Create Build Directory
```powershell
cd C:\Projects\order-book-visualizer
mkdir build
cd build
```

### Configure CMake (First Time)
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cmake .. -G "MinGW Makefiles"
```
**Output on success:**
```
-- The CXX compiler identification is GNU 15.1.0
-- Configuring done
-- Generating done
-- Build files have been written to: .../build
```

### Build the Project
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cmake --build . --target orderbook
```
**Output on success:**
```
[ 16%] Building CXX object CMakeFiles/orderbook.dir/src/main.cpp.obj
[ 33%] Building CXX object CMakeFiles/orderbook.dir/src/Order.cpp.obj
...
[100%] Linking CXX executable bin\orderbook.exe
[100%] Built target orderbook
```

### Clean and Rebuild
```powershell
cmake --build . --clean-first --target orderbook
```

### One-Line Build Command
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"; cd C:\Projects\order-book-visualizer\build; cmake --build . --target orderbook
```

### Verify Build Output
```powershell
dir bin\orderbook.exe
# Should show ~300KB executable
```

### Delete CMake Cache (If Configuration Issues)
```powershell
cd C:\Projects\order-book-visualizer\build
Remove-Item -Recurse -Force *
cmake .. -G "MinGW Makefiles"
cmake --build . --target orderbook
```

---

## 4. Node.js Frontend Commands

### Create React Project (First Time)
```powershell
cd C:\Projects\order-book-visualizer\frontend
npm create vite@latest . -- --template react-ts
```
**When prompted:** Select "React" then "TypeScript"

### Install Dependencies
```powershell
npm install
npm install recharts tailwindcss postcss autoprefixer
npm install lucide-react react-router-dom
```

### Initialize Tailwind
```powershell
npx tailwindcss init -p
```

### Start Development Server
```powershell
npm run dev
```
**Output:**
```
  VITE v5.x.x  ready in xxx ms
  ➜  Local:   http://localhost:5173/
```

### Build for Production
```powershell
npm run build
```
**Output:** Creates `dist/` folder with optimized files

### Preview Production Build
```powershell
npm run preview
```

### Clean Node Modules (If Issues)
```powershell
cd C:\Projects\order-book-visualizer\frontend
Remove-Item -Recurse -Force node_modules
Remove-Item package-lock.json
npm install
```

---

## 5. Server Startup Commands

### C++ Backend Server
```powershell
# Set PATH and start server
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cd C:\Projects\order-book-visualizer\build\bin
.\orderbook.exe --headless --auto-start
```
**Output on success:**
```
[INFO] Starting MarketPulse WebSocket Server...
[INFO] Headless mode: No console visualization
[INFO] Auto-starting simulation
[INFO] WebSocket server running on ws://localhost:8080
[INFO] Ready to accept connections
```

**Other modes:**
```powershell
.\orderbook.exe --help           # Show usage help
.\orderbook.exe                   # With console visualization
.\orderbook.exe --headless        # Headless, manual start
```

### React Frontend
```powershell
cd C:\Projects\order-book-visualizer\frontend
npm run dev
```
**Or use npm.cmd explicitly:**
```powershell
npm.cmd run dev
```

### Verify Both Running
```powershell
# Check backend (should show orderbook.exe)
Get-Process -Name orderbook

# Check frontend (should show node)
Get-Process -Name node
```

### Stop Servers
```powershell
# Stop backend
Get-Process -Name orderbook | Stop-Process -Force

# Stop frontend (Ctrl+C in terminal, or:)
Get-Process -Name node | Stop-Process -Force
```

---

## 6. Common Error Resolutions

### Error: 'g++' is not recognized
```
g++ : The term 'g++' is not recognized as the name of a cmdlet...
```
**Fix:** Add MinGW to PATH first:
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
g++ --version  # Now works
```

### Error: CMake generator not found
```
CMake Error: Could not create named generator MinGW Makefiles
```
**Fix:** Ensure using correct CMake:
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
# The above adds MSYS2's cmake which knows about MinGW
```

### Error: libwebsockets not found
```
Package libwebsockets was not found in the pkg-config search path
```
**Fix:** Install in MSYS2 terminal:
```bash
pacman -S mingw-w64-x86_64-libwebsockets
```

### Error: npm.ps1 cannot be loaded (PSSecurityException)
```
File C:\Program Files\nodejs\npm.ps1 cannot be loaded because running 
scripts is disabled on this system.
```
**Fix Option 1:** Use npm.cmd instead:
```powershell
npm.cmd run dev
```

**Fix Option 2:** Change execution policy:
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### Error: WebSocket connection refused
```
WebSocket connection to 'ws://localhost:8080' failed
```
**Fix:** Start C++ backend first:
```powershell
cd C:\Projects\order-book-visualizer\build\bin
.\orderbook.exe --headless --auto-start
```

### Error: Port 8080 already in use
```
[ERROR] Failed to create WebSocket context: Address already in use
```
**Fix:** Kill existing process:
```powershell
Get-Process -Name orderbook | Stop-Process -Force
# Or find what's using port:
netstat -ano | findstr :8080
taskkill /PID <pid> /F
```

### Error: Frontend can't connect after server restart
**Fix:** Refresh browser tab (Ctrl+R) after restarting backend.

### Error: DLL not found (libwebsockets.dll)
```
The code execution cannot proceed because libwebsockets.dll was not found
```
**Fix:** Run with MinGW in PATH:
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
.\orderbook.exe --headless --auto-start
```

---

## 7. Quick Reference Commands

### Complete Build Sequence (First Time)
```powershell
# 1. Configure
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cd C:\Projects\order-book-visualizer\build
cmake .. -G "MinGW Makefiles"

# 2. Build
cmake --build . --target orderbook

# 3. Install frontend deps
cd ..\frontend
npm install

# 4. Done! Now run with:
# Terminal 1: .\build\bin\orderbook.exe --headless --auto-start
# Terminal 2: npm run dev (in frontend folder)
```

### Daily Development (Quick Start)
**Terminal 1 - Backend:**
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cd C:\Projects\order-book-visualizer\build\bin
.\orderbook.exe --headless --auto-start
```

**Terminal 2 - Frontend:**
```powershell
cd C:\Projects\order-book-visualizer\frontend
npm run dev
```

**Browser:**
```
http://localhost:5173
```

### Rebuild After Code Changes
```powershell
# If C++ changed:
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cd C:\Projects\order-book-visualizer\build
cmake --build . --target orderbook

# Frontend auto-reloads with Vite (no rebuild needed)
```

### Test WebSocket with Browser Console
```javascript
// In browser DevTools console:
ws = new WebSocket('ws://localhost:8080', ['lws-minimal']);
ws.onopen = () => console.log('Connected!');
ws.onmessage = e => console.log(JSON.parse(e.data));
ws.send(JSON.stringify({type:'start',config:{symbol:'BTC/USD',startPrice:50000}}));
```

### Kill All Project Processes
```powershell
Get-Process -Name orderbook -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process -Name node -ErrorAction SilentlyContinue | Stop-Process -Force
```

### Check Port Usage
```powershell
netstat -ano | Select-String ":8080"
netstat -ano | Select-String ":5173"
```

---

## Build History (Chronological Sessions)

### Session 1: Initial Environment Setup
```
[✓] Installed MSYS2 at C:\msys64
[✓] Ran pacman -Syu (package database update)
[✓] Installed mingw-w64-x86_64-toolchain
[✓] Installed mingw-w64-x86_64-cmake  
[✓] Installed mingw-w64-x86_64-libwebsockets
[✓] Verified: g++ --version → GCC 15.1.0
```

### Session 2: Project Structure Creation
```
[✓] mkdir include src build frontend tests
[✓] Created CMakeLists.txt with libwebsockets
[✓] Created header files in include/
[✓] Created source files in src/
```

### Session 3: First C++ Build
```
[✓] cmake .. -G "MinGW Makefiles" - Configured
[✓] cmake --build . --target orderbook - Built
[✓] bin/orderbook.exe created (~300KB)
[✓] Server starts and listens on port 8080
```

### Session 4: Frontend Setup
```
[✓] npm create vite@latest . -- --template react-ts
[✓] npm install (base dependencies)
[✓] npm install recharts tailwindcss postcss autoprefixer
[✓] npm install lucide-react react-router-dom
[✓] npx tailwindcss init -p
[✓] npm run dev - Server running at localhost:5173
```

### Session 5: WebSocket Integration
```
[✓] Frontend connects to ws://localhost:8080
[✓] Protocol 'lws-minimal' handshake successful
[✓] Tick messages flowing at ~100ms intervals
[✓] All control messages working
```

### Session 6: Candlestick Charts
```
[✓] Custom SVG candlestick rendering
[✓] Timeframe selector implemented
[✓] Volume bars aligned with candles
[✓] Candle history request/response working
[✓] Candle caching (500 per timeframe)
```

### Session 7: Session Isolation
```
[✓] Per-client SessionState in C++
[✓] Independent PriceEngine per session
[✓] Independent CandleManager per session
[✓] Multiple browser tabs work independently
```

### Session 8: Final Polish
```
[✓] Ping/pong latency display in header
[✓] Educational tooltips with Portal
[✓] Removed all debug console.log
[✓] News shock system working
[✓] All sentiments/intensities tested
```

---

## Troubleshooting Checklist

If something doesn't work, check these in order:

1. **PATH set?** Run `$env:Path = "C:\msys64\mingw64\bin;$env:Path"`
2. **Backend running?** Check `Get-Process -Name orderbook`
3. **Correct port?** Backend: 8080, Frontend: 5173
4. **Browser refreshed?** After backend restart, refresh tab
5. **Dependencies installed?** Run `npm install` in frontend folder
6. **Build successful?** Check for errors in cmake output
7. **DLLs available?** MinGW PATH must be set when running .exe

---

*End of Build Log*
