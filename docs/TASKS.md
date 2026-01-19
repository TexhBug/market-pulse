# MarketPulse - Complete Build Tasks

> **Project**: MarketPulse (Real-Time Order Book Visualizer)  
> **Last Updated**: 2026-01-18  
> **Status**: ✅ Complete (All phases done)

This document contains step-by-step instructions to BUILD this project from scratch.
Each task includes what to do, why, and how to verify it worked.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Phase 1: Environment Setup](#phase-1-environment-setup)
3. [Phase 2: C++ Server Foundation](#phase-2-c-server-foundation)
4. [Phase 3: WebSocket & Session System](#phase-3-websocket--session-system)
5. [Phase 4: React Frontend](#phase-4-react-frontend)
6. [Phase 5: Frontend Components](#phase-5-frontend-components)
7. [Phase 6: Candlestick & Timeframes](#phase-6-candlestick--timeframes)
8. [Phase 7: Advanced Features](#phase-7-advanced-features)
9. [Phase 8: Polish & Bug Fixes](#phase-8-polish--bug-fixes)
10. [Issues & Resolutions Log](#issues--resolutions-log)
11. [Quick Start](#quick-start)

---

## Prerequisites

Before starting, ensure you have:
- **Windows 10/11** (this guide uses PowerShell)
- **Internet connection** (for downloads)
- **~5GB free disk space** (for tools and dependencies)

---

## Phase 1: Environment Setup

### Task 1.1: Install MSYS2
```powershell
# Download MSYS2 from https://www.msys2.org/
# Or use winget:
winget install -e --id MSYS2.MSYS2 --accept-package-agreements --accept-source-agreements
```
**Verify:** MSYS2 installed at `C:\msys64\`

### Task 1.2: Install MinGW Toolchain (in MSYS2 terminal)
```bash
# Open MSYS2 MINGW64 terminal and run:
pacman -Syu
pacman -S mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-libwebsockets
```
**Verify:** `g++` available at `C:\msys64\mingw64\bin\g++.exe`

### Task 1.3: Install CMake
```powershell
winget install -e --id Kitware.CMake --accept-package-agreements --accept-source-agreements
```
**Verify:** `cmake --version` shows version 4.x+

### Task 1.4: Install Node.js
```powershell
winget install -e --id OpenJS.NodeJS.LTS --accept-package-agreements --accept-source-agreements
```
**Verify:** `node --version` shows version 18+

### Task 1.5: Add MinGW to PATH (every PowerShell session)
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
```
**Verify:** `g++ --version` works from PowerShell

### Task 1.6: Create Project Directory
```powershell
mkdir C:\Projects\order-book-visualizer
cd C:\Projects\order-book-visualizer
mkdir include src build frontend tests
```

### Task 1.7: Verify All Tools
```powershell
g++ --version     # Should show GCC 15.x+
cmake --version   # Should show CMake 4.x+
node --version    # Should show v18+
npm --version     # Should show v10+
```

---

## Phase 2: C++ Server Foundation

### Task 2.1: Create CMakeLists.txt
Create `CMakeLists.txt` in project root:
```cmake
cmake_minimum_required(VERSION 3.16)
project(orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBSOCKETS REQUIRED libwebsockets)

add_executable(orderbook
    src/main.cpp
    src/Order.cpp
    src/OrderQueue.cpp
    src/OrderBook.cpp
    src/MatchingEngine.cpp
    src/Visualizer.cpp
    src/WebSocketServer.cpp
)

target_include_directories(orderbook PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${WEBSOCKETS_INCLUDE_DIRS}
)

target_link_libraries(orderbook
    ${WEBSOCKETS_LIBRARIES}
    ws2_32
)
```

### Task 2.2: Create Core Header Files

**include/Common.h** - Shared types
```cpp
#pragma once
#include <string>
#include <cstdint>

struct TradeData { 
    int64_t id; 
    double price; 
    double quantity; 
    std::string side; 
    int64_t timestamp; 
};

struct Candle { 
    int64_t timestamp; 
    double open, high, low, close, volume; 
    int trades; 
};

enum class Side { BUY, SELL };
enum class OrderType { LIMIT, MARKET };
```

**include/Order.h** - Order structure
**include/OrderBook.h** - Order book with bids/asks
**include/OrderQueue.h** - Thread-safe queue
**include/MatchingEngine.h** - Matching logic
**include/Visualizer.h** - Console display
**include/WebSocketServer.h** - libwebsockets wrapper with JsonBuilder
**include/MarketSentiment.h** - Sentiment types and parameters
**include/NewsShock.h** - News shock controller

### Task 2.3: Create Source Files

Implement all .cpp files in `src/`:
- `main.cpp` - Entry point with simulation loop
- `Order.cpp` - Order methods
- `OrderQueue.cpp` - Thread-safe queue
- `OrderBook.cpp` - Order book methods
- `MatchingEngine.cpp` - Matching logic
- `Visualizer.cpp` - Console rendering
- `WebSocketServer.cpp` - libwebsockets implementation

### Task 2.4: Initial Build Test
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cd C:\Projects\order-book-visualizer\build
cmake .. -G "MinGW Makefiles"
cmake --build . --target orderbook
```
**Verify:** `build\bin\orderbook.exe` created

### Task 2.5: Test C++ Server
```powershell
.\bin\orderbook.exe --help
.\bin\orderbook.exe --headless --auto-start
```
**Verify:** Server starts and listens on port 8080

---

## Phase 3: WebSocket & Session System

### Task 3.1: Implement WebSocketServer

Key requirements:
- Use `lws-minimal` as protocol name (MUST match frontend)
- Create per-connection session state
- Handle all message types
- Send batched tick messages with all data

### Task 3.2: Implement Session Isolation

Each WebSocket client gets independent:
- **PriceEngine** - Calculates price movement with pullbacks
- **CandleManager** - Tracks OHLCV candles for 5 timeframes
- **OrderBook** - 15 bid/ask levels
- **NewsShockController** - Cooldown system for price shocks
- **Stats** - Volume, trades, high/low

### Task 3.3: Implement Message Handlers

**Client → Server Messages:**

| Message Type | Handler                                      |
|--------------|----------------------------------------------|
| start        | Initialize session with config               |
| sentiment    | Update session sentiment (BULLISH, etc.)     |
| intensity    | Update volatility multiplier                 |
| spread       | Update bid-ask spread                        |
| speed        | Update simulation speed                      |
| pause        | Pause/resume price updates                   |
| reset        | Reset session to initial state               |
| newsShock    | Enable/disable news shock system             |
| getCandles   | Return cached candles for timeframe          |
| ping         | Respond with pong + timestamp                |

### Task 3.4: Implement Tick Message

Send every ~100ms containing:
- `orderbook` - bids, asks, bestBid, bestAsk, spread
- `stats` - currentPrice, sentiment, intensity, paused, etc.
- `price` - timestamp, price, volume
- `currentCandles` - incomplete candles for all timeframes
- `completedCandles` - newly completed candles or null
- `trade` - if generated (~33% chance)

### Task 3.5: Test WebSocket

Use browser console:
```javascript
ws = new WebSocket('ws://localhost:8080', ['lws-minimal']);
ws.onmessage = e => console.log(JSON.parse(e.data));
ws.send(JSON.stringify({type:'start',config:{}}));
```
**Verify:** Tick messages arrive every ~100ms

---

## Phase 4: React Frontend

### Task 4.1: Create React App with Vite
```powershell
cd C:\Projects\order-book-visualizer\frontend
npm create vite@latest . -- --template react-ts
npm install
```

### Task 4.2: Install Dependencies
```powershell
npm install recharts tailwindcss postcss autoprefixer
npm install lucide-react react-router-dom
npx tailwindcss init -p
```

### Task 4.3: Configure Tailwind

**tailwind.config.js:**
```javascript
export default {
  content: ['./index.html', './src/**/*.{js,ts,jsx,tsx}'],
  theme: { extend: {} },
  plugins: [],
}
```

**src/index.css:**
```css
@tailwind base;
@tailwind components;
@tailwind utilities;
```

### Task 4.4: Create Type Definitions

**src/types.ts:**
```typescript
export interface OrderLevel { 
  price: number; 
  quantity: number; 
  total: number; 
}

export interface OrderBookData { 
  bids: OrderLevel[]; 
  asks: OrderLevel[]; 
  bestBid: number; 
  bestAsk: number; 
  spread: number; 
}

export interface Trade { 
  id: string; 
  price: number; 
  quantity: number; 
  side: 'buy' | 'sell'; 
  timestamp: number; 
}

export interface MarketStats { 
  currentPrice: number; 
  volume24h: number; 
  high24h: number; 
  low24h: number; 
  trades24h: number; 
  priceChange24h: number; 
  sentiment: string; 
  intensity: string; 
  paused: boolean; 
  newsShockEnabled: boolean;
}

export interface PricePoint { 
  timestamp: number; 
  price: number; 
  volume: number; 
}

export interface Candle { 
  timestamp: number; 
  open: number; 
  high: number; 
  low: number; 
  close: number; 
  volume: number; 
}
```

### Task 4.5: Create useWebSocket Hook

**src/hooks/useWebSocket.ts:**
- Establish WebSocket connection with protocol `lws-minimal`
- Handle all message types (tick, started, candleHistory, etc.)
- Maintain state for orderBook, trades, stats, candles
- Implement ping/pong latency measurement
- Provide methods: connect, disconnect, send

### Task 4.6: Create Router

**src/App.tsx:**
```tsx
import { BrowserRouter, Routes, Route } from 'react-router-dom';
import LandingPage from './pages/LandingPage';
import VisualizerPage from './pages/VisualizerPage';

export default function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<LandingPage />} />
        <Route path="/visualizer" element={<VisualizerPage />} />
      </Routes>
    </BrowserRouter>
  );
}
```

### Task 4.7: Test Frontend
```powershell
npm run dev
# Open http://localhost:5173
```
**Verify:** Page loads, shows landing page

---

## Phase 5: Frontend Components

### Task 5.1: Create Header Component
- Logo and title
- Connection status indicator (green/red dot)
- Latency display with color coding:
  - Green: < 50ms
  - Yellow: < 100ms
  - Red: > 100ms

### Task 5.2: Create Order Book Component
- 15 bid levels (green, depth bars on right)
- 15 ask levels (red, depth bars on right)
- Use `flex-col-reverse` for asks so best price is near spread
- Spread display in middle divider
- Cumulative totals column

### Task 5.3: Create Control Panel
- Sentiment selector (6 buttons with colors/icons)
- Intensity selector (5 buttons)
- Spread input ($0.05 - $0.25)
- Speed slider (0.5x - 2x)
- Pause/Resume button (toggle icon)
- Reset button
- News shock toggle with cooldown display

### Task 5.4: Create Stats Panel
- Current price with UP/DOWN color
- 24h high/low range
- 24h volume
- 24h trades count
- Price change percentage

### Task 5.5: Create Trade Ticker
- Last 30 trades (configurable: 8, 10, 15, 20)
- Price, quantity, side (buy=green/sell=red)
- Timestamp in HH:MM:SS format
- Flash animation on new trade

### Task 5.6: Create Landing Page
- Symbol input (default: BTC/USD)
- Starting price input (default: 50000)
- Start button → navigates to /visualizer

### Task 5.7: Create Visualizer Page
Layout:
```
┌─────────────────────────────────────────────────────┐
│                     Header                          │
├───────────────────────────┬─────────────────────────┤
│                           │      Order Book         │
│    Candlestick Chart      │      Stats Panel        │
│                           │      Control Panel      │
├───────────────────────────┴─────────────────────────┤
│                    Trade Ticker                     │
└─────────────────────────────────────────────────────┘
```

---

## Phase 6: Candlestick & Timeframes

### Task 6.1: Create Candlestick Chart Component
- Custom SVG-based OHLC candles (NOT Recharts candlestick)
- Fixed candle width (4px) and spacing (6px)
- Y-axis width (50px) on left
- Green candles: close > open
- Red candles: close < open
- Wicks showing high/low

### Task 6.2: Add Timeframe Selector
- Tick (raw price line)
- 1 second
- 5 seconds
- 30 seconds
- 1 minute
- 5 minutes

### Task 6.3: Add Volume Bars
- Below price chart (30% of height)
- Aligned with candlestick positions
- Green/red matching candle color

### Task 6.4: Add Chart Type Toggle
- Line chart (for tick data)
- Candlestick chart (for timeframes)

### Task 6.5: Implement Candle Caching (Server)
- Store completed candles per timeframe
- Max 500 candles per timeframe
- Send `candleHistory` on timeframe request
- Send `candleReset` on simulation reset

### Task 6.6: Implement Candle Caching (Client)
- `candleCache` state: 500 candles per timeframe
- `currentCandles` state: incomplete candles
- Request history via `getCandles` on timeframe switch
- Append completed candles from tick messages

---

## Phase 7: Advanced Features

### Task 7.1: Implement Probability-Based Price Movement
Replace simple drift with:
- Bullish: 65% chance UP, 35% DOWN
- Bearish: 35% chance UP, 65% DOWN
- Others: 50/50

### Task 7.2: Implement Pullback System
- Track consecutive moves in same direction
- After 8-15 moves, force counter-trend pullback
- Pullback lasts 2-5 ticks
- Pullback strength: 70-90% of normal

### Task 7.3: Implement News Shock System
- Toggle button enables 5-second active window
- 3% chance per tick during window
- 6-12x normal price movement
- 20-second cooldown after window
- Server-side enforcement (client can't cheat)

### Task 7.4: Implement Educational Mode
- Context provider for enabled state
- Tooltip component using React Portal
- Explanations for Order Book, Trade Ticker, Chart

### Task 7.5: Add Ping/Pong Latency
- Send ping with timestamp every 2 seconds
- Measure RTT when pong received
- Display in header with color coding

---

## Phase 8: Polish & Bug Fixes

### Task 8.1: Fix Trade ID Uniqueness
Generate unique IDs across sessions:
```cpp
tradeId = sessionId * 1000000 + tradeCounter;
```

### Task 8.2: Fix Pause Functionality
- Server stops sending price updates when paused
- Client doesn't add to trades when paused
- Order book shows frozen state

### Task 8.3: Remove Debug Logging
Remove all `console.log` statements from production code

### Task 8.4: Fix Recharts Warning
Add `minWidth={100}` to all ResponsiveContainer components

### Task 8.5: Suppress Ping Logging
Don't print "ping" messages to server console

### Task 8.6: Update Help Text
Match actual intensity multipliers (0.4x/0.7x/1.0x/1.2x/1.6x)
Use correct sentiment names (SIDEWAYS not CALM)

### Task 8.7: Add `started` Message
Server sends `{"type":"started"}` when simulation begins

### Task 8.8: Final Testing
- [ ] Multiple tabs work independently
- [ ] Pause/resume affects both client and server
- [ ] All controls work
- [ ] Chart timeframe switching loads history
- [ ] No console errors
- [ ] Latency displays correctly

---

## Issues & Resolutions Log

### Issue #1: C++ libwebsockets Handshake Failure
**Symptom:** Browser clients connect then immediately disconnect
**Error:** "Invalid header token"
**Resolution:** Protocol name must be `lws-minimal` on both sides

### Issue #2: React useEffect Double-Mount
**Symptom:** WebSocket connecting twice on component mount
**Cause:** React StrictMode mounts components twice in development
**Resolution:** Use `useRef` to track if already connected

### Issue #3: TypeScript Error in Tooltip
**Symptom:** Build error on Recharts Tooltip formatter
**Error:** Parameter 'value' implicitly has 'any' type
**Resolution:** Use type inference: `(value) => value != null ? ... : ''`

### Issue #4: Price Movement Too Aggressive
**Symptom:** Bullish aggressive/extreme caused prices to skyrocket
**Resolution:** Reduced intensity multipliers (EXTREME from 2.5x to 1.6x)

### Issue #5: Chart Disappearing on Window Shrink
**Symptom:** Candlestick chart collapses to 0 height
**Resolution:** Added minHeight constraints and overflow-auto

### Issue #6: Trades Mixing Between Tabs
**Symptom:** Trade from session 1 appears in session 2
**Cause:** broadcastTrade() was sending to all clients
**Resolution:** Removed broadcastTrade, include trade in tick message

### Issue #7: Tooltips Clipped by Overflow
**Symptom:** Tooltips cut off near container edges
**Resolution:** Use React Portal to render at document.body level

### Issue #8: Volume Bars Not Aligned
**Symptom:** Volume bars 2px left of candlesticks
**Resolution:** Add `+ barWidth/2` to volume bar centerX calculation

---

## Quick Start

After all tasks completed, here's how to run:

### Terminal 1: C++ Server
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cd C:\Projects\order-book-visualizer\build\bin
.\orderbook.exe --headless --auto-start
```

### Terminal 2: Frontend
```powershell
cd C:\Projects\order-book-visualizer\frontend
npm run dev
```

### Browser
Open http://localhost:5173

---

## Task Checklist Summary

| Phase | Tasks | Status |
|-------|-------|--------|
| 1. Environment Setup | 7 tasks | ✅ Complete |
| 2. C++ Foundation | 5 tasks | ✅ Complete |
| 3. WebSocket System | 5 tasks | ✅ Complete |
| 4. React Frontend | 7 tasks | ✅ Complete |
| 5. Frontend Components | 7 tasks | ✅ Complete |
| 6. Candlestick Charts | 6 tasks | ✅ Complete |
| 7. Advanced Features | 5 tasks | ✅ Complete |
| 8. Polish & Fixes | 8 tasks | ✅ Complete |

**Total: 50+ tasks completed**

---

*End of Build Tasks*
