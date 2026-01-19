# MarketPulse - Complete Knowledge Base

> **Project**: MarketPulse (Real-Time Order Book Visualizer)  
> **Location**: `C:\Projects\order-book-visualizer`  
> **Last Updated**: 2026-01-18

This document contains EVERYTHING you need to UNDERSTAND to rebuild this project from scratch.
Written to be understandable even if you're new to these technologies!

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Build Tools & Environment](#2-build-tools--environment)
3. [C++ Concepts](#3-c-concepts)
4. [Order Book Concepts](#4-order-book-concepts)
5. [System Architecture](#5-system-architecture)
6. [C++ Backend Details](#6-c-backend-details)
7. [React Frontend Details](#7-react-frontend-details)
8. [Sentiment & Intensity System](#8-sentiment--intensity-system)
9. [Price Movement System](#9-price-movement-system)
10. [Candlestick & Timeframes](#10-candlestick--timeframes)
11. [Candle Caching System](#11-candle-caching-system)
12. [Multi-Session Architecture](#12-multi-session-architecture)
13. [News Shock System](#13-news-shock-system)
14. [WebSocket Protocol](#14-websocket-protocol)
15. [File Structure](#15-file-structure)
16. [Configuration Reference](#16-configuration-reference)
17. [Troubleshooting Guide](#17-troubleshooting-guide)
18. [Glossary](#18-glossary)

---

## 1. Project Overview

### What Is This?

A real-time market order book simulator with:
- **C++ WebSocket server** (high-performance backend using libwebsockets)
- **React/TypeScript frontend** (visualization with Recharts)
- **Per-client session isolation** (multiple browser tabs work independently)
- **Candlestick charts** with multiple timeframes (Tick, 1s, 5s, 30s, 1m, 5m)
- **Simulated price movement** with 6 market sentiments and 5 intensity levels
- **News shock events** that cause dramatic price spikes

### Tech Stack

| Layer    | Technologies                                    |
|----------|------------------------------------------------|
| Backend  | C++17, libwebsockets 4.4.0, CMake, MinGW-w64   |
| Frontend | React 18, TypeScript, Vite, Tailwind CSS, Recharts |

### Ports

| Service          | URL                      |
|------------------|--------------------------|
| WebSocket Server | `ws://localhost:8080`    |
| Frontend Dev     | `http://localhost:5173`  |

---

## 2. Build Tools & Environment

Think of these as the "kitchen appliances" that help us cook our code into a working program!

### MinGW-w64 (Minimalist GNU for Windows)

**What is it?**
Imagine you write a letter in English, but your computer only understands binary (0s and 1s). MinGW-w64 is like a translator that converts your C++ code into something the computer can actually run!

```
Your C++ Code  →  [MinGW-w64/g++]  →  Program.exe (that you can run!)
```

**What's inside?**
- `g++` : The main translator (compiler) for C++ code
- `gcc` : Same thing but for C code
- `gdb` : A detective tool to find bugs in your code (debugger)

**Why not just use Notepad?**
Notepad can only edit text. g++ actually turns that text into a program your computer can execute. It's the difference between having a recipe and actually cooking the meal!

### MSYS2 (Minimal System 2)

**What is it?**
Think of MSYS2 as an "App Store" for programming tools on Windows. Instead of downloading tools from random websites, you use MSYS2's package manager to install everything safely and easily.

**The magic command - pacman:**
```bash
pacman -S mingw-w64-x86_64-gcc     # Install the C++ compiler
pacman -S mingw-w64-x86_64-make    # Install the build tool
```

The "-S" means "sync/install" - easy to remember as "S for Setup"!

**Where does it live?**
- Usually at: `C:\msys64\`
- The compiler ends up at: `C:\msys64\mingw64\bin\g++.exe`

### CMake - The Project Manager

**What is it?**
Imagine you're building IKEA furniture. CMake is like the instruction manual that tells you which pieces go together and in what order.

For code, it figures out:
- Which files need to be compiled
- In what order
- With what settings

**Example CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.16)
project(OrderBookVisualizer)        # Our project name

add_executable(orderbook            # Create orderbook.exe
    src/main.cpp                    # Using these source files
    src/OrderBook.cpp
)
```

**The CMake workflow (just 2 steps!):**
```bash
# Step 1: Configure (create the build instructions)
cmake -B build -G "MinGW Makefiles"

# Step 2: Build (actually compile everything)
cmake --build build
```

**Why use CMake instead of just calling g++ directly?**
With just g++, you'd have to type:
```bash
g++ -o program.exe file1.cpp file2.cpp file3.cpp file4.cpp -lpthread -Wall
```
And remember that EVERY. SINGLE. TIME.

With CMake, you just write the recipe once, then: `cmake --build build` - Done!

---

## 3. C++ Concepts

### Multithreading (Running things in parallel)

**What is it?**
Imagine a restaurant kitchen:
- Single-threaded: ONE chef does everything - chops veggies, then cooks meat, then makes sauce, then plates. Slow!
- Multi-threaded: MULTIPLE chefs work at the same time - one chops, one cooks, one plates. Fast!

**In our Order Book:**
- Thread 1: Receives incoming orders from the market
- Thread 2: Processes and matches orders
- Thread 3: Updates the visual display

All happening AT THE SAME TIME!

**Code Example:**
```cpp
#include <thread>
#include <iostream>

void processOrders() {
    std::cout << "Processing orders..." << std::endl;
}

void updateDisplay() {
    std::cout << "Updating screen..." << std::endl;
}

int main() {
    // Start both tasks at the same time!
    std::thread worker1(processOrders);
    std::thread worker2(updateDisplay);
    
    // Wait for both to finish
    worker1.join();
    worker2.join();
    
    return 0;
}
```

### Mutex (The Bathroom Lock)

**What is it?**
Imagine a single bathroom shared by multiple people. You need a LOCK to make sure only ONE person uses it at a time. Otherwise... chaos!

A mutex (mutual exclusion) is that lock for your data.

**The Problem it solves:**
```
Without a mutex:
   Thread 1: Reads account balance ($100)
   Thread 2: Reads account balance ($100)
   Thread 1: Adds $50, writes $150
   Thread 2: Subtracts $30, writes $70    ← WRONG! Should be $120!

With a mutex:
   Thread 1: LOCKS, reads $100, adds $50, writes $150, UNLOCKS
   Thread 2: Waits... LOCKS, reads $150, subtracts $30, writes $120, UNLOCKS ✓
```

**Code Example:**
```cpp
#include <mutex>

std::mutex orderBookLock;  // Our "bathroom lock"

void addOrder(Order order) {
    // Lock the door before entering
    std::lock_guard<std::mutex> guard(orderBookLock);
    
    // Now we're safe - only we can modify the order book
    orderBook.add(order);
    
    // Lock automatically releases when we leave this function
}
```

### Key Data Structures

**std::map - The Sorted Filing Cabinet**
- Keeps everything in order (sorted by key)
- Great for: Price levels (highest bids first, lowest asks first)
- Finding things: Pretty fast (O(log n))

```cpp
std::map<double, int> priceToQuantity;
priceToQuantity[100.50] = 500;  // 500 shares at $100.50
priceToQuantity[100.00] = 300;  // 300 shares at $100.00
// Automatically sorted: $100.00 comes before $100.50
```

**std::unordered_map - The Hash Table (Super Fast Lookup)**
- Not sorted, but SUPER fast to find things
- Great for: Finding an order by its ID
- Finding things: Almost instant! (O(1))

```cpp
std::unordered_map<int, Order> orderById;
orderById[12345] = myOrder;
Order found = orderById[12345];  // Boom! Found it!
```

**std::queue - The Line at a Coffee Shop**
- First In, First Out (FIFO)
- Great for: Processing orders in the order they arrived

```cpp
std::queue<Order> incomingOrders;
incomingOrders.push(order1);  // Customer 1 joins line
incomingOrders.push(order2);  // Customer 2 joins line
Order next = incomingOrders.front();  // Serve customer 1 first
incomingOrders.pop();                  // Customer 1 leaves
```

---

## 4. Order Book Concepts

### What is an Order Book?

Imagine a marketplace:
- BUYERS shouting: "I'll buy 100 apples for $1 each!"
- SELLERS shouting: "I'll sell 50 apples for $1.10 each!"

An Order Book is simply an organized list of all these shouts!

```
╔═══════════════════════════════════════════════════════╗
║              ORDER BOOK - AAPL Stock                  ║
╠═══════════════════════════════════════════════════════╣
║     BIDS (Buyers)      │      ASKS (Sellers)          ║
║   "I want to BUY"      │    "I want to SELL"          ║
╠═══════════════════════════════════════════════════════╣
║                        │  $150.25  x  100 shares      ║
║                        │  $150.20  x  250 shares      ║
║                        │  $150.15  x  500 shares ←ASK ║
║  ─────────────────── SPREAD ($0.10) ──────────────    ║
║  $150.05  x  300 ←BID │                               ║
║  $150.00  x  450      │                               ║
║  $149.95  x  200      │                               ║
╚═══════════════════════════════════════════════════════╝

Best Bid: $150.05 (highest price someone will pay)
Best Ask: $150.15 (lowest price someone will sell)
Spread:   $0.10   (the gap between them)
```

**Key Points:**
- BIDS are sorted HIGHEST first (best price for sellers at top)
- ASKS are sorted LOWEST first (best price for buyers at top)
- When Bid >= Ask, a trade happens!

### Order Book Display Layout

```
FRONTEND DISPLAY (OrderBook.tsx):
┌─────────────────────────────────────┐
│  ASK (Sell Orders)    ← Red         │
│  $100.50  (worst ask)               │  ↑ Uses flex-col-reverse
│  $100.45                            │  │ so lowest ask appears
│  $100.40                            │  │ at bottom (near spread)
│  $100.35                            │  │
│  $100.30                            │  │
│  $100.25                            │  │
│  $100.20                            │  │
│  $100.15                            │  │
│  $100.10                            │  │
│  $100.05  (best ask)  ← Near spread │
├─────────────────────────────────────┤
│  Best Bid: $99.95                   │  ← SPREAD DIVIDER
│  Best Ask: $100.05                  │
│  Spread: $0.10                      │
├─────────────────────────────────────┤
│  BID (Buy Orders)     ← Green       │
│  $99.95   (best bid)  ← Near spread │  Natural array order
│  $99.90                             │  (highest first)
│  $99.85                             │
│  $99.80                             │
│  ...                                │
│  $99.50   (worst bid)               │
└─────────────────────────────────────┘

The best prices MEET IN THE MIDDLE at the spread divider.
This is the standard layout used by Binance, Coinbase, etc.
```

### Order Types

**LIMIT ORDER - "I want THIS price or better"**
- Example: "Buy 100 shares at $150.00 or less"
- Your order waits in the book until someone matches it
- Patient approach - you might get a better price!

**MARKET ORDER - "I want it NOW, any price!"**
- Example: "Buy 100 shares RIGHT NOW at whatever the best price is"
- Executes immediately against the best available price
- Fast, but you might pay more

### Matching Engine - The Matchmaker

The matching engine is like a dating app for orders - it finds buyers and sellers who are compatible and pairs them up!

**How it works:**
1. New SELL order arrives: "Sell 100 at $150.00"
2. Engine checks BIDS: "Is anyone willing to pay $150.00 or MORE?"
   - Best Bid is $150.05 ← YES! They'll pay even more!
3. MATCH! Trade happens at $150.05
   - Buyer pays $150.05 (they were willing to pay up to $150.05)
   - Seller receives $150.05 (they wanted at least $150.00)
4. If the sell order isn't fully filled, it goes into the book as an ASK

### Bid-Ask Spread

**What is the spread?**
The spread is the difference between the best bid (highest buy price) and the best ask (lowest sell price).

```
Best Bid: $99.95  (highest someone is willing to pay)
Best Ask: $100.00 (lowest someone is willing to sell)
Spread:   $0.05   (the gap between them)
```

**Why does spread matter?**
- Small Spread ($0.01): Orders match quickly, price moves rapidly
- Large Spread ($0.25): Orders rarely match immediately, levels build up

---

## 5. System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              SYSTEM ARCHITECTURE                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────┐        WebSocket         ┌──────────────────────┐        │
│   │   Browser   │ ◄────────────────────►   │   C++ Server         │        │
│   │   Tab 1     │    ws://localhost:8080   │                      │        │
│   └─────────────┘                          │  ┌────────────────┐  │        │
│                                            │  │ Session 1      │  │        │
│   ┌─────────────┐                          │  │ - PriceEngine  │  │        │
│   │   Browser   │ ◄───────────────────►    │  │ - OrderBook    │  │        │
│   │   Tab 2     │                          │  │ - CandleManager│  │        │
│   └─────────────┘                          │  └────────────────┘  │        │
│                                            │                      │        │
│   ┌─────────────┐                          │  ┌────────────────┐  │        │
│   │   Browser   │ ◄───────────────────►    │  │ Session 2      │  │        │
│   │   Tab 3     │                          │  │ - PriceEngine  │  │        │
│   └─────────────┘                          │  │ - OrderBook    │  │        │
│                                            │  │ - CandleManager│  │        │
│                                            │  └────────────────┘  │        │
│                                            └──────────────────────┘        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Data Flow (per tick, ~100ms)

1. Server calculates new price based on sentiment/intensity
2. Server updates candles for all timeframes (1s, 5s, 30s, 1m, 5m)
3. Server generates order book (15 bids, 15 asks)
4. Server may generate a trade (~33% chance)
5. Server sends batched "tick" message to client
6. Client updates UI (chart, order book, stats, trades)

---

## 6. C++ Backend Details

### Key Header Files (`include/`)

| File                | Purpose                                    |
|---------------------|-------------------------------------------|
| `Common.h`          | Shared types (TradeData, Candle, etc.)    |
| `WebSocketServer.h` | WebSocket server + JsonBuilder class      |
| `MarketSentiment.h` | Sentiment/intensity logic + parameters    |
| `NewsShock.h`       | News shock controller with cooldown       |
| `OrderBook.h`       | Order book data structure                 |
| `MatchingEngine.h`  | Order matching (for terminal mode)        |
| `OrderQueue.h`      | Thread-safe order queue                   |
| `Visualizer.h`      | Console-based visualization               |

### Key Source Files (`src/`)

| File                  | Purpose                                  |
|-----------------------|------------------------------------------|
| `main.cpp`            | Entry point, simulation loop, commands   |
| `WebSocketServer.cpp` | libwebsockets implementation             |
| `OrderBook.cpp`       | Order book methods                       |
| `MatchingEngine.cpp`  | Order matching logic                     |
| `OrderQueue.cpp`      | Thread-safe queue implementation         |
| `Visualizer.cpp`      | Console display functions                |

### Session State (Per Client)

Each WebSocket connection gets its own independent:
- **PriceEngine** - Calculates price movement with pullbacks
- **CandleManager** - Tracks OHLCV for 5 timeframes
- **OrderBook** - 15 bid/ask levels
- **NewsShockController** - Cooldown system for price shocks
- **Stats** - Volume, trades, high/low, etc.

### JsonBuilder Class

Helper class for building JSON strings without a JSON library:
```cpp
class JsonBuilder {
public:
    void startObject();
    void endObject();
    void startArray(const std::string& key);
    void endArray();
    void addString(const std::string& key, const std::string& value);
    void addNumber(const std::string& key, double value);
    void addBool(const std::string& key, bool value);
    std::string build();
};
```

**CRITICAL**: C++ JSON must have NO trailing commas! JavaScript allows them, C++ must handle this carefully.

---

## 7. React Frontend Details

### Key Files

| Path                              | Purpose                         |
|-----------------------------------|---------------------------------|
| `src/App.tsx`                     | Router setup                    |
| `src/hooks/useWebSocket.ts`       | WebSocket connection + state    |
| `src/components/Header.tsx`       | Connection status + latency     |
| `src/components/CandlestickChart.tsx` | OHLC chart with volume      |
| `src/components/OrderBook.tsx`    | Bid/ask depth display           |
| `src/components/ControlPanel.tsx` | Sentiment/intensity controls    |
| `src/components/StatsPanel.tsx`   | Market statistics               |
| `src/components/TradeTicker.tsx`  | Recent trades list              |
| `src/pages/LandingPage.tsx`       | Configuration form              |
| `src/pages/VisualizerPage.tsx`    | Main dashboard                  |

### State Management (useWebSocket hook)

| State          | Type            | Purpose                    |
|----------------|-----------------|----------------------------|
| connected      | boolean         | WebSocket connected?       |
| latency        | number \| null  | RTT in milliseconds        |
| orderBook      | OrderBookData   | Current bid/ask levels     |
| trades         | Trade[]         | Last 30 trades             |
| stats          | MarketStats     | Price, volume, sentiment   |
| priceHistory   | PricePoint[]    | Last 100 ticks             |
| candleCache    | CandleCache     | 500 candles per timeframe  |
| currentCandles | CurrentCandles  | Incomplete candles         |

### TypeScript Interfaces (types.ts)

```typescript
interface OrderLevel { price: number; quantity: number; total: number; }
interface OrderBookData { bids: OrderLevel[]; asks: OrderLevel[]; bestBid: number; bestAsk: number; spread: number; }
interface Trade { id: string; price: number; quantity: number; side: 'buy' | 'sell'; timestamp: number; }
interface MarketStats { currentPrice: number; volume24h: number; high24h: number; low24h: number; trades24h: number; priceChange24h: number; sentiment: string; intensity: string; paused: boolean; newsShockEnabled: boolean; }
interface PricePoint { timestamp: number; price: number; volume: number; }
interface Candle { timestamp: number; open: number; high: number; low: number; close: number; volume: number; }
```

### Tailwind CSS - Utility-First Styling

Instead of writing CSS files, you add small utility classes directly to HTML:

```jsx
// Traditional CSS requires separate file
<button class="error-button">Delete</button>

// Tailwind - no separate CSS file needed!
<button class="bg-red-500 text-white px-4 py-2 rounded">Delete</button>
```

Common classes we use:
- Layout: `flex`, `grid`, `col-span-4`, `gap-4`
- Spacing: `p-4` (padding), `m-2` (margin), `space-y-2`
- Colors: `bg-slate-800`, `text-green-400`, `border-gray-700`
- Text: `text-sm`, `font-bold`, `text-center`
- Effects: `hover:bg-slate-700`, `transition`

### Chart Configuration

| Setting              | Value    |
|----------------------|----------|
| Candle width         | 4px      |
| Candle spacing       | 6px      |
| Y-axis width         | 50px     |
| Max cached candles   | 500      |
| Timeframes           | Tick, 1s, 5s, 30s, 1m, 5m |

---

## 8. Sentiment & Intensity System

### Sentiment Types

Each sentiment affects price movement, trend bias, and order book depth:

| Sentiment | Up Prob | Volatility | Description                        |
|-----------|---------|------------|------------------------------------|
| BULLISH   | 65%     | 0.0005     | "Higher highs, higher lows"        |
| BEARISH   | 35%     | 0.0005     | "Lower highs, lower lows"          |
| VOLATILE  | 50%     | 0.0015     | Large swings, frequent reversals   |
| SIDEWAYS  | 50%     | 0.0002     | Range-bound, very low movement     |
| CHOPPY    | 50%     | 0.001      | Erratic, unpredictable             |
| NEUTRAL   | 50%     | 0.0004     | Balanced, baseline behavior        |

**Order Book Effects:**
- BULLISH: 1.5x bid depth, 0.7x ask depth (more buyers)
- BEARISH: 0.7x bid depth, 1.5x ask depth (more sellers)
- VOLATILE: 0.6x both (thin, low liquidity)
- SIDEWAYS: 1.3x both (thick, high liquidity)

### Intensity Levels

Intensity MULTIPLIES the effects of sentiment:

| Intensity  | Price Multiplier | Volume Multiplier |
|------------|------------------|-------------------|
| MILD       | 0.4x             | 0.5x              |
| MODERATE   | 0.7x             | 0.8x              |
| NORMAL     | 0.85x            | 1.0x              |
| AGGRESSIVE | 1.0x             | 1.2x              |
| EXTREME    | 1.25x            | 1.5x              |

**Note**: Values were reduced to create more realistic price movement. AGGRESSIVE (was 1.2x) and EXTREME (was 1.6x) are now more balanced.

### Example Combinations

- BULLISH + EXTREME = Noticeable upward trend
- BEARISH + AGGRESSIVE = Moderate sell-off
- VOLATILE + EXTREME = Larger swings both ways
- SIDEWAYS + MILD = Almost flat line

---

## 9. Price Movement System

### The Problem with Simple Drift

Old approach: `price += drift + random_noise`

This creates UNREALISTIC movement:
- Straight lines up or down
- No pullbacks or pauses
- Doesn't look like real charts

### The Solution: Probability + Forced Pullbacks

Two components combine for realistic movement:

**1. PROBABILITY-BASED DIRECTION**
- Each tick has a % chance of going up vs down
- Bullish: 65% UP, 35% DOWN
- Bearish: 35% UP, 65% DOWN
- Others: 50/50 balanced

**2. FORCED PULLBACKS**
- After 8-15 consecutive moves in trend direction
- Force 2-5 ticks of counter-trend movement
- Pullback moves are slightly smaller (70-90% strength)
- Creates realistic "steps" in the chart

### Visual Comparison

```
OLD (Linear Drift):          NEW (Probability + Pullbacks):

     ╱                           ╱╲
    ╱                          ╱  ╲╱╲
   ╱                         ╱      ╲╱╲
  ╱                        ╱          ╲╱╲
 ╱                       ╱              ╲╱
╱                       ╱
Straight line up        Steps with pullbacks
```

### Pullback Parameters

| Sentiment  | Up Prob | Pullback Threshold | Pullback Duration |
|------------|---------|--------------------|--------------------|
| Bullish    | 65%     | 8-15 ticks         | 2-5 ticks          |
| Bearish    | 35%     | 8-15 ticks         | 2-5 ticks          |
| Volatile   | 50%     | 8-15 ticks         | 2-5 ticks          |
| Sideways   | 50%     | 8-15 ticks         | 2-5 ticks          |
| Choppy     | 50%     | 8-15 ticks         | 2-5 ticks          |
| Neutral    | 50%     | 8-15 ticks         | 2-5 ticks          |

---

## 10. Candlestick & Timeframes

### Candlestick Anatomy

Each "candle" shows FOUR prices for a time period: Open, High, Low, Close (OHLC).

```
    │         ← High (highest price during period)
    │
  ┌─┴─┐       ← Body top (Open or Close, whichever is higher)
  │   │
  │   │       ← Body shows price range between Open and Close
  │   │
  └─┬─┘       ← Body bottom (Open or Close, whichever is lower)
    │
    │         ← Low (lowest price during period)
```

**Colors:**
- GREEN candle: Price went UP (Close > Open) - Bullish!
- RED candle: Price went DOWN (Close < Open) - Bearish!

### Timeframes

| Timeframe  | Use Case                      | Typical Traders     |
|------------|-------------------------------|---------------------|
| Tick       | Every price update as a point | HFT, Scalpers       |
| 1 second   | Ultra high-frequency detail   | Scalpers            |
| 5 seconds  | Very short-term movements     | Day traders         |
| 30 seconds | Short-term patterns           | Active traders      |
| 1 minute   | Intraday trading              | Day traders         |
| 5 minutes  | Balanced intraday view        | Swing traders       |

### Volume

Volume = Total number of shares/units traded during a time period.
- High volume = lots of trading activity = significant price move
- Low volume = few trades = less conviction in the move

**Volume + Price = Confirmation:**
- Price UP + High Volume = Strong bullish move (buyers in control)
- Price UP + Low Volume = Weak move (might reverse)
- Price DOWN + High Volume = Strong bearish move (sellers in control)
- Price DOWN + Low Volume = Weak move (might reverse)

---

## 11. Candle Caching System

### The Problem

Storing raw price ticks for long timeframes is memory-intensive:
- 100ms updates = 10 ticks/second = 600 ticks/minute
- To show 50 five-minute candles, you'd need 150,000 raw price points!

### The Solution: Server-Side Candle Caching

Instead of storing raw ticks, we cache completed candles:
- N price points → Just 5 values per candle (O, H, L, C, volume)
- ~40 bytes per candle vs hundreds of bytes for raw data

### How It Works

```
SERVER STATE:
─────────────
priceHistory: [100 recent ticks]

candleCache: {
  1:   [500 x 1s candles]
  5:   [500 x 5s candles]
  30:  [500 x 30s candles]
  60:  [500 x 1m candles]
  300: [500 x 5m candles]
}

currentCandles: {
  1:   { current 1s candle }
  5:   { current 5s candle }
  ...
}
```

### Message Flow

1. Every price tick → Server updates currentCandles for ALL timeframes

2. When a candle period ends:
   - currentCandle moves to candleCache
   - Server sends: `{ type: 'candle', data: { timeframe, candle } }`
   - Client adds to its cache

3. When client changes timeframe:
   - Client sends: `{ type: 'getCandles', timeframe: 300 }`
   - Server responds: `{ type: 'candleHistory', data: { candles, current } }`

4. On reset:
   - Server sends: `{ type: 'candleReset' }`
   - Client clears all cached candles

### Data Limits

| Timeframe  | Max Cached | Time Coverage |
|------------|------------|---------------|
| Tick       | 100 ticks  | ~10 seconds   |
| 1 second   | 500 candles| ~8 minutes    |
| 5 seconds  | 500 candles| ~42 minutes   |
| 30 seconds | 500 candles| ~4 hours      |
| 1 minute   | 500 candles| ~8 hours      |
| 5 minutes  | 500 candles| ~42 hours     |

---

## 12. Multi-Session Architecture

### How It Works

```
                ┌─────────────────────┐
                │   WebSocket Server  │
                │     (port 8080)     │
                └──────────┬──────────┘
                           │
       ┌───────────────────┼───────────────────┐
       │                   │                   │
       ▼                   ▼                   ▼
┌────────────┐      ┌────────────┐      ┌────────────┐
│  Session 1 │      │  Session 2 │      │  Session 3 │
│ AAPL $150  │      │ MSFT $400  │      │ DEMO $100  │
│  Bullish   │      │  Bearish   │      │  Volatile  │
└────────────┘      └────────────┘      └────────────┘
       │                   │                   │
       ▼                   ▼                   ▼
┌────────────┐      ┌────────────┐      ┌────────────┐
│  Browser 1 │      │  Browser 2 │      │  Browser 3 │
└────────────┘      └────────────┘      └────────────┘
```

Each browser tab gets its OWN:
- Price simulation (independent price movement)
- Order book (separate bid/ask levels)
- Trade history (own trades list)
- Controls (pause/play affects only that session)
- Configuration (symbol, sentiment, intensity)

### Session Lifecycle

1. **CONNECT**: Browser opens ws://localhost:8080
   - Server creates new session with unique ID
   - Session gets fresh simulation state

2. **CONFIGURE**: Browser sends config (symbol, price, sentiment...)
   - Server initializes that session's state
   - Simulation starts for that session only

3. **RUN**: Server broadcasts updates to that client only
   - Order book, trades, stats, price points
   - ~100ms update interval

4. **CONTROL**: Browser sends commands (pause, reset, change sentiment...)
   - Server updates ONLY that session's state
   - Other sessions unaffected

5. **DISCONNECT**: Browser closes or navigates away
   - Server stops that session's simulation loop
   - Session state is cleaned up
   - Other sessions continue running

---

## 13. News Shock System

### Concept

In real markets, prices don't always move gradually. Sometimes:
- A company announces surprising earnings → stock gaps up 10%
- Fed raises rates unexpectedly → market drops instantly
- Breaking news hits → volatility spikes

News Shock simulates these sudden, dramatic price movements.

### Lifecycle

```
READY STATE            ACTIVE WINDOW           COOLDOWN PERIOD
────────────           ─────────────           ───────────────
Button: Gray           Button: Amber           Button: Disabled
Badge: "OFF"           Badge: "4s"→"3s"...     Badge: "18s"→"17s"...
Clickable: Yes         Clickable: No           Clickable: No

        │                    │                        │
        ▼                    ▼                        ▼
    ┌───────┐           ┌─────────┐              ┌─────────┐
    │ Click │──────────▶│ 5 sec   │─────────────▶│ 20 sec  │───▶ READY
    │       │           │ Active  │              │Cooldown │
    └───────┘           └─────────┘              └─────────┘
                             │
                             ▼
                  3% chance per tick (10/sec)
                  = ~1.5 shocks per activation
                             │
                             ▼
                  SHOCK! Price moves 6-12x normal
```

### Shock Parameters

| Parameter              | Value         | Description                    |
|------------------------|---------------|--------------------------------|
| Active Window          | 5 seconds     | Duration shocks can occur      |
| Cooldown Period        | 20 seconds    | Wait before next activation    |
| Trigger Probability    | 3% per tick   | ~1.5 shocks per 5s window      |
| Min Gap Between Shocks | 2 seconds     | Prevents back-to-back shocks   |
| Shock Multiplier       | 6-12x         | Price move size vs normal      |
| Direction              | 50/50         | Equally likely up or down      |

### Shock Magnitude Comparison

| Scenario                | Move Size | Example ($100 stock) |
|-------------------------|-----------|----------------------|
| Normal tick (NORMAL)    | 0.034%    | $100.00 → $100.03    |
| EXTREME intensity       | 0.05%     | $100.00 → $100.05    |
| NEWS SHOCK (min)        | 0.24%     | $100.00 → $100.24    |
| NEWS SHOCK (max)        | 0.48%     | $100.00 → $100.48    |
| NEWS SHOCK (average)    | 0.36%     | $100.00 → $100.36    |

**Relative Impact:** Even at EXTREME intensity (1.25x), shock is 4.8-9.6x more impactful.

---

## 14. WebSocket Protocol

### Protocol Name

**CRITICAL**: Protocol name MUST be `lws-minimal` and MUST match between frontend and backend!

```javascript
// Frontend
const ws = new WebSocket('ws://localhost:8080', ['lws-minimal']);
```

### Client → Server Messages

| Type       | Example                                        | Description                    |
|------------|------------------------------------------------|--------------------------------|
| start      | `{"type":"start","config":{...}}`              | Initialize simulation          |
| sentiment  | `{"type":"sentiment","value":"BULLISH"}`       | Change market sentiment        |
| intensity  | `{"type":"intensity","value":"AGGRESSIVE"}`    | Change volatility level        |
| spread     | `{"type":"spread","value":"0.10"}`             | Set bid-ask spread             |
| speed      | `{"type":"speed","value":"2"}`                 | Change simulation speed        |
| pause      | `{"type":"pause","value":"true"}`              | Pause/resume simulation        |
| reset      | `{"type":"reset","value":"true"}`              | Reset to initial state         |
| newsShock  | `{"type":"newsShock","value":"true"}`          | Enable news shock mode         |
| getCandles | `{"type":"getCandles","timeframe":"60"}`       | Request candle history         |
| ping       | `{"type":"ping","value":"1737225600000"}`      | Latency measurement            |

### Server → Client Messages

| Type          | Description                              |
|---------------|------------------------------------------|
| tick          | Batched: orderbook, stats, price, candles|
| started       | Confirms simulation started              |
| candleHistory | Response to getCandles request           |
| candleReset   | Signal to clear candle cache             |
| pong          | Response to ping with timestamp          |

### Tick Message Structure

```json
{
  "type": "tick",
  "data": {
    "orderbook": { 
      "bids": [{"price": 99.95, "quantity": 150, "total": 150}, ...], 
      "asks": [{"price": 100.05, "quantity": 120, "total": 120}, ...], 
      "bestBid": 99.95, 
      "bestAsk": 100.05, 
      "spread": 0.10 
    },
    "stats": { 
      "currentPrice": 100.00, 
      "sentiment": "BULLISH", 
      "intensity": "NORMAL", 
      "paused": false, 
      "newsShockEnabled": false,
      "newsShockCooldown": false,
      "newsShockCooldownRemaining": 0,
      "newsShockActiveRemaining": 0,
      "volume24h": 12345,
      "high24h": 102.50,
      "low24h": 98.20,
      "trades24h": 567,
      "priceChange24h": 1.25
    },
    "price": { "timestamp": 1737225600000, "price": 100.00, "volume": 150 },
    "currentCandles": { 
      "1": {"timestamp": 1737225600000, "open": 99.95, "high": 100.05, "low": 99.90, "close": 100.00, "volume": 450},
      "5": {...}, 
      "30": {...}, 
      "60": {...}, 
      "300": {...} 
    },
    "completedCandles": [{"timeframe": 1, "candle": {...}}, ...] or null,
    "trade": { "id": "1000001", "price": 100.00, "quantity": 50, "side": "buy", "timestamp": 1737225600000 } or null
  }
}
```

---

## 15. File Structure

```
order-book-visualizer/
├── CMakeLists.txt              # Build configuration
├── KNOWLEDGE.md                # This file (what to KNOW)
├── TASKS.md                    # Step-by-step build guide (what to DO)
├── BUILD_LOG.md                # Command history (what was RUN)
│
├── include/                    # C++ headers
│   ├── Common.h                # Shared types, enums
│   ├── MarketSentiment.h       # Sentiment/intensity parameters
│   ├── MatchingEngine.h        # Order matching logic
│   ├── NewsShock.h             # News shock controller
│   ├── Order.h                 # Order structure
│   ├── OrderBook.h             # Order book class
│   ├── OrderQueue.h            # Thread-safe queue
│   ├── Visualizer.h            # Console display
│   └── WebSocketServer.h       # WebSocket + JsonBuilder
│
├── src/                        # C++ source files
│   ├── main.cpp                # Entry point, simulation loop
│   ├── MatchingEngine.cpp      # Matching implementation
│   ├── Order.cpp               # Order methods
│   ├── OrderBook.cpp           # Order book methods
│   ├── OrderQueue.cpp          # Queue implementation
│   ├── Visualizer.cpp          # Console rendering
│   └── WebSocketServer.cpp     # libwebsockets implementation
│
├── build/                      # CMake build output
│   └── bin/
│       └── orderbook.exe       # Compiled executable
│
├── frontend/                   # React application
│   ├── package.json            # NPM dependencies
│   ├── vite.config.ts          # Vite configuration
│   ├── tsconfig.json           # TypeScript config
│   ├── tailwind.config.js      # Tailwind CSS config
│   ├── index.html              # Entry HTML
│   └── src/
│       ├── App.tsx             # React Router setup
│       ├── main.tsx            # Entry point
│       ├── index.css           # Tailwind imports
│       ├── types.ts            # TypeScript interfaces
│       ├── hooks/
│       │   └── useWebSocket.ts # WebSocket connection hook
│       ├── components/
│       │   ├── Header.tsx              # Connection status + latency
│       │   ├── CandlestickChart.tsx    # OHLC chart + volume
│       │   ├── OrderBook.tsx           # Bid/ask depth
│       │   ├── ControlPanel.tsx        # Controls
│       │   ├── StatsPanel.tsx          # Statistics
│       │   ├── TradeTicker.tsx         # Recent trades
│       │   └── EducationalTooltip.tsx  # Tooltip component
│       ├── pages/
│       │   ├── LandingPage.tsx         # Config form
│       │   └── VisualizerPage.tsx      # Main dashboard
│       └── context/
│           └── EducationalModeContext.tsx
│
└── tests/                      # Unit tests (GoogleTest)
    ├── CMakeLists.txt
    ├── test_order.cpp
    ├── test_orderbook.cpp
    ├── test_matching_engine.cpp
    └── test_order_queue.cpp
```

---

## 16. Configuration Reference

### C++ Server

| Setting              | Value                      | Location                 |
|----------------------|----------------------------|--------------------------|
| WebSocket port       | 8080                       | main.cpp                 |
| Protocol name        | `lws-minimal`              | WebSocketServer.cpp      |
| Tick interval        | 50ms base (adjusted)       | main.cpp                 |
| Max clients          | Unlimited                  | -                        |
| Candle timeframes    | 1, 5, 30, 60, 300 seconds  | Common.h                 |
| Max cached candles   | 500 per timeframe          | main.cpp                 |
| Order book depth     | 15 levels each side        | main.cpp                 |

### Frontend

| Setting              | Value                      | Location                 |
|----------------------|----------------------------|--------------------------|
| Dev server port      | 5173                       | vite.config.ts           |
| WebSocket URL        | `ws://localhost:8080`      | useWebSocket.ts          |
| Ping interval        | 2000ms                     | useWebSocket.ts          |
| Max trades shown     | 30                         | useWebSocket.ts          |
| Max price history    | 100 ticks                  | useWebSocket.ts          |

### Command Line Options (C++ Server)

```
Usage: orderbook.exe [OPTIONS]

Options:
  -i, --interactive       Interactive setup (prompts for all options)
  -p, --price <value>     Base price ($100 - $500, default: $100)
  -s, --symbol <name>     Stock symbol (default: DEMO)
  --spread <value>        Initial spread ($0.05 - $0.25, default: $0.05)
  --sentiment <type>      bullish|bearish|volatile|sideways|choppy|neutral
  --intensity <level>     mild|moderate|normal|aggressive|extreme
  --speed <value>         Speed multiplier (0.25 - 2.0)
  -a, --auto-start        Skip 'press any key' prompt
  --headless              Run without terminal UI (for WebSocket mode)
  --debug                 Enable verbose logging
  -h, --help              Show help
```

### Runtime Keyboard Controls (Console Mode)

| Key       | Action                                      |
|-----------|---------------------------------------------|
| 1-6       | Switch sentiment (1=Bull 2=Bear 3=Vol ...)  |
| M/N/A/X   | Set intensity (M=Mild N=Normal A=Aggressive)|
| + / -     | Increase/decrease spread                    |
| P         | Pause/Resume simulation                     |
| F / S     | Faster/Slower speed                         |
| Q / ESC   | Quit                                        |

---

## 17. Troubleshooting Guide

### Build Errors

| Error                              | Solution                                           |
|------------------------------------|----------------------------------------------------|
| `g++ not found`                    | Add MinGW to PATH: `$env:Path = "C:\msys64\mingw64\bin;$env:Path"` |
| `cmake not found`                  | Install via MSYS2: `pacman -S mingw-w64-x86_64-cmake` |
| `libwebsockets not found`          | Install: `pacman -S mingw-w64-x86_64-libwebsockets` |
| `undefined reference to lws_*`     | Check CMakeLists.txt has correct link libraries    |
| `Permission denied (can't write .exe)` | Kill running process first: `taskkill /F /IM orderbook.exe` |

### WebSocket Issues

| Issue                              | Solution                                           |
|------------------------------------|----------------------------------------------------|
| Connection failed                  | Check server is running on port 8080               |
| Invalid header token               | Protocol name mismatch - must be `lws-minimal`     |
| Connection closes immediately      | Check for JSON errors (no trailing commas in C++)  |
| Connects then disconnects          | Check netstat for TIME_WAIT buildup                |

### Frontend Issues

| Issue                              | Solution                                           |
|------------------------------------|----------------------------------------------------|
| npm: scripts disabled              | Use `npm.cmd` instead of `npm` on Windows          |
| Recharts width(-1) warning         | Add `minWidth={100}` to ResponsiveContainer        |
| Trades mixing between tabs         | Use unique trade IDs: `sessionId * 1000000 + counter` |
| React double-mount (StrictMode)    | Use `useRef` to track if already connected         |
| TypeScript 'any' type error        | Add explicit types or use type inference           |

### Common Gotchas

1. **Always rebuild C++ after changes**: `cmake --build . --target orderbook`
2. **Frontend hot-reloads**, but WebSocket reconnect needs page refresh
3. **Pause must freeze BOTH** server updates AND client state changes
4. **JSON from C++ must have NO trailing commas**
5. **PATH must be set every new PowerShell session** (or make permanent)

---

## 18. Glossary

| Term       | Definition                                                |
|------------|----------------------------------------------------------|
| Bid        | An offer to BUY at a certain price                        |
| Ask        | An offer to SELL at a certain price                       |
| Spread     | The gap between best bid and best ask                     |
| OHLC       | Open, High, Low, Close - four prices defining a candle    |
| Compile    | Convert human-readable code into computer-executable      |
| Link       | Connect compiled pieces together into one program         |
| Thread     | A separate path of execution (like a worker)              |
| Mutex      | A lock to prevent multiple threads from conflicting       |
| FIFO       | First In, First Out (like a queue/line)                   |
| Header     | A .h file that describes what functions/classes exist     |
| Source     | A .cpp file that contains the actual code implementation  |
| Executable | A program you can run (.exe on Windows)                   |
| WebSocket  | Real-time bidirectional communication protocol            |
| Latency    | Time delay between sending and receiving (round-trip)     |
| Timeframe  | Duration each candlestick represents                      |
| Sentiment  | Market mood/direction (bullish, bearish, etc.)            |
| Intensity  | How strongly the sentiment affects price movement         |
| Pullback   | Counter-trend movement after extended directional move    |

---

## Quick Start Commands

### Start Everything (Two Terminals)

**Terminal 1 - C++ Server:**
```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cd C:\Projects\order-book-visualizer\build\bin
.\orderbook.exe --headless --auto-start
```

**Terminal 2 - Frontend:**
```powershell
cd C:\Projects\order-book-visualizer\frontend
npm.cmd run dev
```

**Browser:**
Open http://localhost:5173

---

*End of Knowledge Base*
