# MarketPulse - System Architecture

> **Project**: MarketPulse (Real-Time Order Book Visualizer)  
> **Purpose**: Complete technical architecture documentation  
> **Audience**: Developers rebuilding or extending the project

---

## Table of Contents

1. [System Overview](#1-system-overview)
2. [High-Level Architecture](#2-high-level-architecture)
3. [C++ Backend Architecture](#3-c-backend-architecture)
4. [Threading Model](#4-threading-model)
5. [Synchronization & Concurrency](#5-synchronization--concurrency)
6. [Session Management](#6-session-management)
7. [Data Flow](#7-data-flow)
8. [React Frontend Architecture](#8-react-frontend-architecture)
9. [WebSocket Protocol](#9-websocket-protocol)
10. [Component Interactions](#10-component-interactions)
11. [Why NOT Lock-Free?](#11-why-not-lock-free)
12. [Performance Considerations](#12-performance-considerations)

---

## 1. System Overview

MarketPulse is a **real-time market visualization system** with two main parts:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           MARKETPULSE SYSTEM                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌───────────────────────┐         ┌───────────────────────────────────┐  │
│   │    C++ BACKEND        │ ══════> │       REACT FRONTEND              │  │
│   │    (Server)           │ WebSocket│       (Browser)                  │  │
│   │    Port 8080          │ <══════ │       Port 5173                   │  │
│   └───────────────────────┘         └───────────────────────────────────┘  │
│                                                                             │
│   • Market simulation                • Order book visualization            │
│   • Price engine                     • Candlestick charts                  │
│   • Session management               • Control panel                       │
│   • WebSocket server                 • Real-time updates                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Key Design Principles:**
- Each browser tab = independent simulation session
- Server manages ALL state (client can't cheat)
- Single-threaded event loop for WebSocket (libwebsockets)
- Multi-threaded backend for order generation/processing

---

## 2. High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              COMPLETE SYSTEM DIAGRAM                                │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                     │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │                         C++ BACKEND (orderbook.exe)                          │   │
│  │                                                                              │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ │   │
│  │  │   Order     │  │   Order     │  │  Matching   │  │    Visualizer       │ │   │
│  │  │  Generator  │──│   Queue     │──│   Engine    │──│  (Console Output)   │ │   │
│  │  │  (Thread 1) │  │(Thread-Safe)│  │  (Thread 2) │  │                     │ │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘ │   │
│  │         │                                  │                                 │   │
│  │         └──────────────────┬───────────────┘                                 │   │
│  │                            │                                                 │   │
│  │                            ▼                                                 │   │
│  │              ┌─────────────────────────────┐                                 │   │
│  │              │      WebSocket Server       │◄──────────────────────┐         │   │
│  │              │    (libwebsockets library)  │                       │         │   │
│  │              │       Port 8080             │                       │         │   │
│  │              └──────────────┬──────────────┘                       │         │   │
│  │                             │                                      │         │   │
│  │    ┌────────────────────────┼────────────────────────┐            │         │   │
│  │    │                        │                        │            │         │   │
│  │    ▼                        ▼                        ▼            │         │   │
│  │ ┌──────────────┐     ┌──────────────┐     ┌──────────────┐       │         │   │
│  │ │  Session 1   │     │  Session 2   │     │  Session N   │       │         │   │
│  │ │──────────────│     │──────────────│     │──────────────│       │         │   │
│  │ │ PriceEngine  │     │ PriceEngine  │     │ PriceEngine  │       │         │   │
│  │ │ CandleManager│     │ CandleManager│     │ CandleManager│       │         │   │
│  │ │ OrderBook    │     │ OrderBook    │     │ OrderBook    │       │         │   │
│  │ │ NewsShock    │     │ NewsShock    │     │ NewsShock    │       │         │   │
│  │ │ Stats        │     │ Stats        │     │ Stats        │       │         │   │
│  │ └──────────────┘     └──────────────┘     └──────────────┘       │         │   │
│  │        │                    │                    │                │         │   │
│  └────────┼────────────────────┼────────────────────┼────────────────┼─────────┘   │
│           │                    │                    │                │             │
│           │ WebSocket          │ WebSocket          │ WebSocket      │ Commands    │
│           ▼                    ▼                    ▼                ▲             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐      │             │
│  │  Browser Tab 1  │  │  Browser Tab 2  │  │  Browser Tab N  │──────┘             │
│  │  (React App)    │  │  (React App)    │  │  (React App)    │                    │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘                    │
│                                                                                    │
└────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. C++ Backend Architecture

### File Organization

```
C++ Backend Structure
─────────────────────

include/                          src/
├── Common.h          ◄──────────►  (shared types)
├── Order.h           ◄──────────►  Order.cpp
├── OrderQueue.h      ◄──────────►  OrderQueue.cpp
├── OrderBook.h       ◄──────────►  OrderBook.cpp
├── MatchingEngine.h  ◄──────────►  MatchingEngine.cpp
├── Visualizer.h      ◄──────────►  Visualizer.cpp
├── WebSocketServer.h ◄──────────►  WebSocketServer.cpp
├── MarketSentiment.h              (header-only)
├── PriceEngine.h                  (header-only)
├── CandleManager.h                (header-only)
├── NewsShock.h                    (header-only)
└── SessionState.h                 (header-only)
                                    main.cpp (entry point)
```

### Class Hierarchy

```
┌───────────────────────────────────────────────────────────────────────────────┐
│                          CLASS DEPENDENCY DIAGRAM                             │
├───────────────────────────────────────────────────────────────────────────────┤
│                                                                               │
│                          ┌─────────────────┐                                  │
│                          │     main()      │                                  │
│                          └────────┬────────┘                                  │
│                                   │                                           │
│          ┌────────────────────────┼────────────────────────┐                  │
│          │                        │                        │                  │
│          ▼                        ▼                        ▼                  │
│  ┌───────────────┐      ┌────────────────┐      ┌─────────────────────┐      │
│  │ OrderGenerator│      │MatchingEngine │      │  WebSocketServer    │      │
│  │   (thread 1)  │      │  (thread 2)   │      │    (thread 3)       │      │
│  └───────┬───────┘      └───────┬────────┘      └──────────┬──────────┘      │
│          │                      │                          │                  │
│          │                      │                          ▼                  │
│          │                      │                ┌─────────────────────┐      │
│          │                      │                │   SessionState      │      │
│          │                      │                │   (per-client)      │      │
│          ▼                      ▼                └──────────┬──────────┘      │
│  ┌───────────────┐      ┌───────────────┐                  │                  │
│  │  OrderQueue   │─────►│   OrderBook   │       ┌──────────┼──────────┐      │
│  │(thread-safe)  │      │               │       │          │          │      │
│  └───────────────┘      └───────────────┘       ▼          ▼          ▼      │
│                                           ┌──────────┐┌─────────┐┌──────────┐│
│                                           │ PriceEng ││CandleMgr││NewsShock ││
│                                           └──────────┘└─────────┘└──────────┘│
│                                                                               │
└───────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Threading Model

MarketPulse uses **4 threads** in the C++ backend:

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                            THREADING MODEL                                      │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  THREAD 1: Order Generator                                                      │
│  ─────────────────────────                                                      │
│  • Runs in infinite loop while g_running == true                                │
│  • Generates buy/sell orders based on sentiment                                 │
│  • Pushes orders to OrderQueue (producer)                                       │
│  • Sleeps between orders (adjusted by speed multiplier)                         │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ void orderGenerator(OrderQueue& queue, ...) {                            │   │
│  │     while (g_running) {                                                  │   │
│  │         if (g_paused) { sleep(100ms); continue; }                        │   │
│  │         Order order = generateOrder();                                   │   │
│  │         queue.push(order);           // ◄── PRODUCER                     │   │
│  │         sleep(delay / speedMultiplier);                                  │   │
│  │     }                                                                    │   │
│  │ }                                                                        │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│  THREAD 2: Order Processor                                                      │
│  ────────────────────────────                                                   │
│  • Waits for orders from OrderQueue (consumer)                                  │
│  • Processes orders through MatchingEngine                                      │
│  • Updates price tracking, generates trades                                     │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ void orderProcessor(OrderQueue& queue, MatchingEngine& engine, ...) {    │   │
│  │     while (g_running) {                                                  │   │
│  │         auto order = queue.popWithTimeout(100);  // ◄── CONSUMER         │   │
│  │         if (order) {                                                     │   │
│  │             auto trades = engine.processOrder(*order);                   │   │
│  │             for (auto& trade : trades) {                                 │   │
│  │                 updatePrice(trade.price);                                │   │
│  │             }                                                            │   │
│  │         }                                                                │   │
│  │     }                                                                    │   │
│  │ }                                                                        │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│  THREAD 3: WebSocket Server                                                     │
│  ───────────────────────────                                                    │
│  • Runs lws_service() in a loop                                                 │
│  • Handles client connections/disconnections                                    │
│  • Receives commands, sends tick updates                                        │
│  • Uses libwebsockets event-driven model                                        │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ void WebSocketServer::serverThread() {                                   │   │
│  │     while (m_running) {                                                  │   │
│  │         lws_service(m_context, 50);  // 50ms timeout                     │   │
│  │     }                                                                    │   │
│  │ }                                                                        │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│  THREAD 4: Keyboard Handler (Console Mode Only)                                 │
│  ──────────────────────────────────────────────                                 │
│  • Polls for keyboard input                                                     │
│  • Updates sentiment/intensity based on key press                               │
│  • Not active in --headless mode                                                │
│                                                                                 │
│  THREAD 5: Display Updater (Session Tick Loop)                                  │
│  ─────────────────────────────────────────────                                  │
│  • Iterates through all active sessions                                         │
│  • Updates each session's price, candles, order book                            │
│  • Sends tick messages to each client                                           │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ void displayUpdater(WebSocketServer* wsServer) {                         │   │
│  │     while (g_running) {                                                  │   │
│  │         for (auto* session : wsServer->getAllSessions()) {               │   │
│  │             if (session->isRunning() && !session->isPaused()) {          │   │
│  │                 updateSessionPrice(session);                             │   │
│  │                 sendTickToClient(session);                               │   │
│  │             }                                                            │   │
│  │         }                                                                │   │
│  │         sleep(100ms);  // ~10 ticks per second                           │   │
│  │     }                                                                    │   │
│  │ }                                                                        │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### Thread Lifecycle

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         THREAD LIFECYCLE DIAGRAM                                │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│   main()                                                                        │
│     │                                                                           │
│     ├──► Create globals (OrderBook, MatchingEngine, etc.)                       │
│     │                                                                           │
│     ├──► wsServer.start() ─────────────► [Thread 3: WebSocket] ──┐              │
│     │                                                             │             │
│     ├──► std::thread(orderGenerator) ──► [Thread 1: Generator] ──┤             │
│     │                                                             │             │
│     ├──► std::thread(orderProcessor) ──► [Thread 2: Processor] ──┤             │
│     │                                                             │             │
│     ├──► std::thread(keyboardHandler) ─► [Thread 4: Keyboard] ───┤             │
│     │                                                             │ Running     │
│     ├──► std::thread(displayUpdater) ──► [Thread 5: Display] ────┤             │
│     │                                                             │             │
│     │                                                             │             │
│     │   [SIGINT received - Ctrl+C]                                │             │
│     │         │                                                   │             │
│     │         ▼                                                   │             │
│     ├──► g_running = false ──────────────────────────────────────►│             │
│     │                                                             │             │
│     │   All threads check g_running and exit their loops          │             │
│     │                                                             │             │
│     ├──► generator.join() ◄──────────────────────────────────────┘             │
│     ├──► processor.join()                                                       │
│     ├──► keyboard.join()                                                        │
│     ├──► display.join()                                                         │
│     ├──► wsServer.stop()                                                        │
│     │                                                                           │
│     └──► return 0;                                                              │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 5. Synchronization & Concurrency

### This Project Uses **Mutex-Based Synchronization** (NOT Lock-Free)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        SYNCHRONIZATION MECHANISMS                               │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  1. OrderQueue - Producer/Consumer Pattern with Mutex                           │
│  ─────────────────────────────────────────────────────                          │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ class OrderQueue {                                                       │   │
│  │ private:                                                                 │   │
│  │     std::queue<Order> m_queue;        // The actual queue                │   │
│  │     mutable std::mutex m_mutex;       // Protects m_queue                │   │
│  │     std::condition_variable m_cv;     // Signals when data available     │   │
│  │     std::atomic<bool> m_shutdown;     // Graceful shutdown flag          │   │
│  │                                                                          │   │
│  │ public:                                                                  │   │
│  │     void push(const Order& order) {                                      │   │
│  │         {                                                                │   │
│  │             std::lock_guard<std::mutex> lock(m_mutex);  // ◄── LOCK      │   │
│  │             m_queue.push(order);                                         │   │
│  │         }                                                                │   │
│  │         m_cv.notify_one();  // Wake up one waiting consumer              │   │
│  │     }                                                                    │   │
│  │                                                                          │   │
│  │     std::optional<Order> pop() {                                         │   │
│  │         std::unique_lock<std::mutex> lock(m_mutex);  // ◄── LOCK         │   │
│  │         m_cv.wait(lock, [this] {                     // ◄── WAIT         │   │
│  │             return !m_queue.empty() || m_shutdown;                       │   │
│  │         });                                                              │   │
│  │         if (m_shutdown && m_queue.empty()) return std::nullopt;          │   │
│  │         Order order = std::move(m_queue.front());                        │   │
│  │         m_queue.pop();                                                   │   │
│  │         return order;                                                    │   │
│  │     }                                                                    │   │
│  │ };                                                                       │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│  2. Client Map - Protected by Mutex                                             │
│  ──────────────────────────────────                                             │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ class WebSocketServer {                                                  │   │
│  │ private:                                                                 │   │
│  │     std::map<uint32_t, ClientData> m_clients;   // Client ID → Data      │   │
│  │     mutable std::mutex m_clientsMutex;          // Protects m_clients    │   │
│  │                                                                          │   │
│  │     void sendToClient(uint32_t clientId, const std::string& msg) {       │   │
│  │         struct lws* wsi = nullptr;                                       │   │
│  │         {                                                                │   │
│  │             std::lock_guard<std::mutex> lock(m_clientsMutex); // ◄── LOCK│   │
│  │             auto it = m_clients.find(clientId);                          │   │
│  │             if (it != m_clients.end()) {                                 │   │
│  │                 it->second.messageQueue.push(msg);                       │   │
│  │                 wsi = it->second.wsi;                                    │   │
│  │             }                                                            │   │
│  │         }  // ◄── UNLOCK before calling libwebsockets                    │   │
│  │         if (wsi) lws_callback_on_writable(wsi);                          │   │
│  │     }                                                                    │   │
│  │ };                                                                       │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│  3. Atomic Variables - For Simple Flags                                         │
│  ──────────────────────────────────────                                         │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ std::atomic<bool> g_running{true};      // Shutdown signal               │   │
│  │ std::atomic<bool> g_paused{false};      // Pause flag                    │   │
│  │ std::atomic<double> g_speedMultiplier;  // Speed control                 │   │
│  │ std::atomic<double> g_currentPrice;     // Current price                 │   │
│  │ std::atomic<double> g_highPrice;        // High price tracking           │   │
│  │ std::atomic<double> g_lowPrice;         // Low price tracking            │   │
│  │                                                                          │   │
│  │ // Atomic compare-exchange for thread-safe min/max updates:              │   │
│  │ double high = g_highPrice.load();                                        │   │
│  │ while (newPrice > high && !g_highPrice.compare_exchange_weak(high, newPrice)) {}│
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│  4. Price Log Mutex                                                             │
│  ─────────────────                                                              │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ std::mutex g_logMutex;                                                   │   │
│  │ std::ofstream g_priceLog;                                                │   │
│  │                                                                          │   │
│  │ void logPrice(double price, const std::string& changeType) {             │   │
│  │     std::lock_guard<std::mutex> lock(g_logMutex);  // ◄── LOCK           │   │
│  │     if (!g_priceLog.is_open()) return;                                   │   │
│  │     g_priceLog << timestamp << ", " << price << ", " << changeType;      │   │
│  │     g_priceLog.flush();                                                  │   │
│  │ }                                                                        │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│  5. Generator Mutex                                                             │
│  ─────────────────                                                              │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ SentimentOrderGenerator* g_generator = nullptr;                          │   │
│  │ std::mutex g_generatorMutex;                                             │   │
│  │                                                                          │   │
│  │ // In orderGenerator thread:                                             │   │
│  │ {                                                                        │   │
│  │     std::lock_guard<std::mutex> lock(g_generatorMutex);                  │   │
│  │     if (g_generator) {                                                   │   │
│  │         genOrder = g_generator->generateOrder();                         │   │
│  │     }                                                                    │   │
│  │ }                                                                        │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### Thread Safety Summary Table

| Shared Resource | Protection | Used By |
|-----------------|------------|---------|
| `OrderQueue` | `std::mutex` + `condition_variable` | Generator → Processor |
| `m_clients` (client map) | `std::mutex` | WebSocket thread, Display thread |
| `g_running`, `g_paused` | `std::atomic<bool>` | All threads |
| `g_currentPrice`, etc. | `std::atomic<double>` | Processor, Display |
| `g_priceLog` (file) | `std::mutex` | Processor, Keyboard |
| `g_generator` | `std::mutex` | Generator, Processor |

---

## 6. Session Management

Each WebSocket client gets an **independent session**:

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           SESSION ARCHITECTURE                                  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  When Client Connects (LWS_CALLBACK_ESTABLISHED):                               │
│  ───────────────────────────────────────────────                                │
│                                                                                 │
│     1. Generate unique client ID                                                │
│     2. Create SessionState with its own:                                        │
│        • PriceEngine (tracks price, pullbacks, sentiment effects)               │
│        • CandleManager (aggregates ticks into candles, caches 500/timeframe)    │
│        • OrderBook (15 bid/ask levels, synthetic for visualization)             │
│        • NewsShockController (5-second window, 20-second cooldown)              │
│        • Stats (open/high/low/current price, volume, trades)                    │
│     3. Store in m_clients map                                                   │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ case LWS_CALLBACK_ESTABLISHED: {                                         │   │
│  │     uint32_t clientId = g_nextClientId++;                                │   │
│  │                                                                          │   │
│  │     ClientData clientData;                                               │   │
│  │     clientData.wsi = wsi;                                                │   │
│  │     clientData.session = std::make_unique<SessionState>(clientId);       │   │
│  │     clientData.ipAddress = getClientIP(wsi);                             │   │
│  │     clientData.connectedAt = now();                                      │   │
│  │                                                                          │   │
│  │     {                                                                    │   │
│  │         std::lock_guard lock(m_clientsMutex);                            │   │
│  │         m_clients[clientId] = std::move(clientData);                     │   │
│  │     }                                                                    │   │
│  │     break;                                                               │   │
│  │ }                                                                        │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│                                                                                 │
│  SessionState Class Structure:                                                  │
│  ─────────────────────────────                                                  │
│                                                                                 │
│  ┌─────────────────────────────────────────────────────────────────────────┐   │
│  │ class SessionState {                                                     │   │
│  │ private:                                                                 │   │
│  │     uint32_t m_sessionId;              // Unique session identifier      │   │
│  │     SessionConfig m_config;            // symbol, basePrice, spread      │   │
│  │     bool m_running, m_paused;          // Session control flags          │   │
│  │                                                                          │   │
│  │     // Price tracking                                                    │   │
│  │     double m_currentPrice, m_openPrice, m_highPrice, m_lowPrice;         │   │
│  │                                                                          │   │
│  │     // Statistics                                                        │   │
│  │     size_t m_totalOrders, m_totalTrades, m_totalVolume;                  │   │
│  │     int64_t m_tradeCounter;                                              │   │
│  │                                                                          │   │
│  │     // INDEPENDENT COMPONENTS (each session has its own)                 │   │
│  │     MarketSentimentController m_sentimentController;                     │   │
│  │     PriceEngine m_priceEngine;                                           │   │
│  │     CandleManager m_candleManager;                                       │   │
│  │     NewsShockController m_newsShockController;                           │   │
│  │     std::unique_ptr<OrderBook> m_orderBook;                              │   │
│  │                                                                          │   │
│  │ public:                                                                  │   │
│  │     // Control methods                                                   │   │
│  │     void setSentiment(Sentiment s);                                      │   │
│  │     void setIntensity(Intensity i);                                      │   │
│  │     void setSpeed(double speed);                                         │   │
│  │     void setPaused(bool paused);                                         │   │
│  │     void reset();                                                        │   │
│  │                                                                          │   │
│  │     // Price engine access                                               │   │
│  │     PriceEngine& getPriceEngine() { return m_priceEngine; }              │   │
│  │     CandleManager& getCandleManager() { return m_candleManager; }        │   │
│  │     OrderBook& getOrderBook() { return *m_orderBook; }                   │   │
│  │                                                                          │   │
│  │     // Trade generation                                                  │   │
│  │     TradeData generateTrade(double price, int64_t timestamp);            │   │
│  │ };                                                                       │   │
│  └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                 │
│                                                                                 │
│  Session Isolation Proof:                                                       │
│  ────────────────────────                                                       │
│                                                                                 │
│    Tab 1: BULLISH + EXTREME              Tab 2: BEARISH + MILD                  │
│    ┌──────────────────────┐              ┌──────────────────────┐               │
│    │ Price: $52,450 (+5%) │              │ Price: $48,200 (-4%) │               │
│    │ Sentiment: BULLISH   │              │ Sentiment: BEARISH   │               │
│    │ Trades: 145          │              │ Trades: 89           │               │
│    │ Chart: trending UP   │              │ Chart: trending DOWN │               │
│    └──────────────────────┘              └──────────────────────┘               │
│                                                                                 │
│    Each tab has completely independent price movement!                          │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 7. Data Flow

### Tick Update Flow (Server → Client)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           TICK DATA FLOW                                        │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│   Every ~100ms, the Display Updater thread does this for EACH session:          │
│                                                                                 │
│   ┌─────────────────────────────────────────────────────────────────────────┐  │
│   │                                                                          │  │
│   │  1. CHECK SESSION STATE                                                  │  │
│   │     ├─ Is running? If no, skip                                           │  │
│   │     └─ Is paused? If yes, skip                                           │  │
│   │                                                                          │  │
│   │  2. UPDATE PRICE                                                         │  │
│   │     ├─ Get sentiment & intensity from session                            │  │
│   │     ├─ Check news shock status                                           │  │
│   │     ├─ Calculate new price via PriceEngine:                              │  │
│   │     │    • Get base move (sentiment direction + volatility)              │  │
│   │     │    • Check pullback (8-15 moves triggers reversal)                 │  │
│   │     │    • Apply intensity multiplier                                    │  │
│   │     │    • Apply news shock if active (6-12x normal)                     │  │
│   │     └─ Update session's currentPrice, high, low                          │  │
│   │                                                                          │  │
│   │  3. UPDATE CANDLES                                                       │  │
│   │     ├─ Add price tick to CandleManager                                   │  │
│   │     ├─ Update current candles for all 5 timeframes                       │  │
│   │     └─ Check if any candles completed (return them)                      │  │
│   │                                                                          │  │
│   │  4. UPDATE ORDER BOOK (Synthetic)                                        │  │
│   │     ├─ Generate 15 bid levels around currentPrice - spread/2             │  │
│   │     └─ Generate 15 ask levels around currentPrice + spread/2             │  │
│   │                                                                          │  │
│   │  5. MAYBE GENERATE TRADE (~33% chance)                                   │  │
│   │     ├─ Determine buy/sell based on sentiment (65% buy if BULLISH)        │  │
│   │     ├─ Generate unique ID: sessionId * 1000000 + counter                 │  │
│   │     └─ Add to session stats                                              │  │
│   │                                                                          │  │
│   │  6. BUILD TICK JSON MESSAGE                                              │  │
│   │     ├─ orderbook: { bids[], asks[], bestBid, bestAsk, spread }           │  │
│   │     ├─ stats: { currentPrice, sentiment, intensity, paused, ... }        │  │
│   │     ├─ price: { timestamp, price, volume }                               │  │
│   │     ├─ currentCandles: { 1: {...}, 5: {...}, ... }                       │  │
│   │     ├─ completedCandles: [...] or null                                   │  │
│   │     └─ trade: {...} or null                                              │  │
│   │                                                                          │  │
│   │  7. SEND TO CLIENT                                                       │  │
│   │     └─ wsServer->sendToClient(clientId, tickJson);                       │  │
│   │                                                                          │  │
│   └─────────────────────────────────────────────────────────────────────────┘  │
│                                                                                 │
│                                                                                 │
│   Sample Tick Message:                                                          │
│   ────────────────────                                                          │
│                                                                                 │
│   {                                                                             │
│     "type": "tick",                                                             │
│     "data": {                                                                   │
│       "orderbook": {                                                            │
│         "bids": [{"price":49950.00,"quantity":1.5}, ...],                       │
│         "asks": [{"price":50050.00,"quantity":2.1}, ...],                       │
│         "bestBid": 49950.00,                                                    │
│         "bestAsk": 50050.00,                                                    │
│         "spread": 100.00                                                        │
│       },                                                                        │
│       "stats": {                                                                │
│         "currentPrice": 50000.00,                                               │
│         "sentiment": "BULLISH",                                                 │
│         "intensity": "NORMAL",                                                  │
│         "paused": false,                                                        │
│         "newsShockEnabled": false,                                              │
│         ...                                                                     │
│       },                                                                        │
│       "price": {"timestamp":1705574400000,"price":50000.00,"volume":100},       │
│       "currentCandles": {                                                       │
│         "1": {"open":49980,"high":50010,"low":49970,"close":50000,...},         │
│         "5": {...}, "30": {...}, "60": {...}, "300": {...}                      │
│       },                                                                        │
│       "completedCandles": null,                                                 │
│       "trade": {"id":1000001,"price":50002.50,"quantity":0.5,"side":"BUY",...}  │
│     }                                                                           │
│   }                                                                             │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### Command Flow (Client → Server)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          COMMAND DATA FLOW                                      │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│   Browser                          Server                                       │
│   ───────                          ──────                                       │
│                                                                                 │
│   User clicks "BULLISH"                                                         │
│         │                                                                       │
│         ▼                                                                       │
│   ws.send({type:"sentiment", value:"BULLISH"})                                  │
│         │                                                                       │
│         │ WebSocket                                                             │
│         ▼                                                                       │
│   LWS_CALLBACK_RECEIVE                                                          │
│         │                                                                       │
│         ▼                                                                       │
│   processMessage(clientId, message)                                             │
│         │                                                                       │
│         ├─► Parse JSON: type="sentiment", value="BULLISH"                       │
│         │                                                                       │
│         ▼                                                                       │
│   m_commandCallback(clientId, "sentiment", "BULLISH")                           │
│         │                                                                       │
│         ▼                                                                       │
│   Command handler in main.cpp:                                                  │
│         │                                                                       │
│         ├─► Find session: wsServer->getSession(clientId)                        │
│         │                                                                       │
│         └─► session->setSentiment(Sentiment::BULLISH)                           │
│                  │                                                              │
│                  └─► m_sentimentController.setSentiment(BULLISH)                │
│                                                                                 │
│   Next tick: session's PriceEngine uses BULLISH parameters                      │
│              (65% up probability, trend direction UP)                           │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 8. React Frontend Architecture

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        REACT COMPONENT HIERARCHY                                │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│   App.tsx (BrowserRouter)                                                       │
│   │                                                                             │
│   ├── LandingPage.tsx                                                           │
│   │   └── Configuration form (symbol, price)                                    │
│   │       └── "Start" button → navigate('/visualizer')                          │
│   │                                                                             │
│   └── VisualizerPage.tsx                                                        │
│       │                                                                         │
│       ├── useWebSocket() hook ◄──────────────────────────────────────┐          │
│       │   │                                                          │          │
│       │   ├── WebSocket connection management                        │          │
│       │   ├── State: orderBook, trades, stats, candles              │          │
│       │   ├── Methods: connect, disconnect, send                     │          │
│       │   └── Ping/pong latency measurement                          │          │
│       │                                                              │          │
│       ├── EducationalModeContext                                     │          │
│       │   └── Provides: { enabled, setEnabled }                      │          │
│       │                                                              │          │
│       └── Layout Components:                                         │          │
│           │                                                          │          │
│           ├── Header.tsx                                             │          │
│           │   ├── Logo, title                                        │          │
│           │   ├── Connection status (green/red dot)                  │          │
│           │   ├── Latency display (colored by ms)                    │          │
│           │   └── Educational mode toggle                            │          │
│           │                                                          │          │
│           ├── CandlestickChart.tsx                                   │          │
│           │   ├── SVG-based candlesticks (NOT Recharts)             │          │
│           │   ├── Volume bars                                        │          │
│           │   ├── Timeframe selector                                 │          │
│           │   ├── Candle cache (500 per timeframe)                   │          │
│           │   └── Price color display (green/red)                    │          │
│           │                                                          │          │
│           ├── OrderBook.tsx                                          │          │
│           │   ├── 15 bid levels (green)                              │          │
│           │   ├── 15 ask levels (red)                                │          │
│           │   ├── Depth bars                                         │          │
│           │   └── Spread display                                     │          │
│           │                                                          │          │
│           ├── StatsPanel.tsx                                         │          │
│           │   ├── Current price with arrow                           │          │
│           │   ├── 24h high/low                                       │          │
│           │   ├── Volume, trades                                     │          │
│           │   └── Price change %                                     │          │
│           │                                                          │          │
│           ├── ControlPanel.tsx                                       │          │
│           │   ├── Sentiment buttons (6)                 ─────────────┘          │
│           │   ├── Intensity buttons (5)                 sends commands          │
│           │   ├── Spread input                          via WebSocket           │
│           │   ├── Speed slider                                                  │
│           │   ├── Pause/Resume                                                  │
│           │   ├── Reset                                                         │
│           │   └── News shock toggle                                             │
│           │                                                                     │
│           ├── TradeTicker.tsx                                                   │
│           │   ├── Last N trades (8/10/15/20)                                    │
│           │   └── Flash animation on new trade                                  │
│           │                                                                     │
│           └── EducationalTooltip.tsx                                            │
│               └── Portal-based tooltip at cursor position                       │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### State Management Flow

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         FRONTEND STATE FLOW                                     │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│   useWebSocket Hook                                                             │
│   ─────────────────                                                             │
│                                                                                 │
│   const {                                                                       │
│     // Connection state                                                         │
│     isConnected,      // boolean - WebSocket connected?                         │
│     latency,          // number - RTT in ms                                     │
│                                                                                 │
│     // Market data (updated every tick)                                         │
│     orderBook,        // { bids[], asks[], spread, ... }                        │
│     stats,            // { currentPrice, sentiment, intensity, ... }            │
│     trades,           // Trade[] - last 30 trades                               │
│     priceHistory,     // PricePoint[] - tick data for line chart                │
│                                                                                 │
│     // Candle data                                                              │
│     candleCache,      // Map<timeframe, Candle[]> - 500 per timeframe           │
│     currentCandles,   // Map<timeframe, Candle> - incomplete candles            │
│                                                                                 │
│     // Methods                                                                  │
│     connect,          // (symbol, price) => void                                │
│     disconnect,       // () => void                                             │
│     send,             // (type, value) => void                                  │
│     requestCandles,   // (timeframe) => void                                    │
│   } = useWebSocket();                                                           │
│                                                                                 │
│                                                                                 │
│   Message Handling:                                                             │
│   ─────────────────                                                             │
│                                                                                 │
│   ws.onmessage = (event) => {                                                   │
│     const msg = JSON.parse(event.data);                                         │
│                                                                                 │
│     switch (msg.type) {                                                         │
│       case 'tick':                                                              │
│         setOrderBook(msg.data.orderbook);                                       │
│         setStats(msg.data.stats);                                               │
│         setPriceHistory(prev => [...prev, msg.data.price].slice(-300));         │
│         if (msg.data.trade) {                                                   │
│           setTrades(prev => [msg.data.trade, ...prev].slice(0, 30));            │
│         }                                                                       │
│         setCurrentCandles(msg.data.currentCandles);                             │
│         if (msg.data.completedCandles) {                                        │
│           // Append to cache                                                    │
│           updateCandleCache(msg.data.completedCandles);                         │
│         }                                                                       │
│         break;                                                                  │
│                                                                                 │
│       case 'candleHistory':                                                     │
│         // Replace cache for requested timeframe                                │
│         setCandleCache(prev => ({                                               │
│           ...prev,                                                              │
│           [msg.data.timeframe]: msg.data.candles                                │
│         }));                                                                    │
│         break;                                                                  │
│                                                                                 │
│       case 'pong':                                                              │
│         const rtt = Date.now() - msg.timestamp;                                 │
│         setLatency(rtt);                                                        │
│         break;                                                                  │
│     }                                                                           │
│   };                                                                            │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 9. WebSocket Protocol

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         WEBSOCKET PROTOCOL                                      │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│   Connection Setup:                                                             │
│   ─────────────────                                                             │
│   • URL: ws://localhost:8080                                                    │
│   • Protocol: 'lws-minimal' (MUST match both sides)                             │
│   • Subprotocol negotiation required by libwebsockets                           │
│                                                                                 │
│   ┌─────────────────────────────────────────────────────────────────────────┐  │
│   │ // Frontend connection                                                   │  │
│   │ const ws = new WebSocket('ws://localhost:8080', ['lws-minimal']);        │  │
│   └─────────────────────────────────────────────────────────────────────────┘  │
│                                                                                 │
│                                                                                 │
│   CLIENT → SERVER Messages:                                                     │
│   ─────────────────────────                                                     │
│                                                                                 │
│   ┌────────────────────┬───────────────────────────────────────────────────┐   │
│   │ Type               │ Example                                           │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ start              │ {"type":"start","config":{"symbol":"BTC/USD",     │   │
│   │                    │  "price":50000,"spread":0.10,"sentiment":"NEUTRAL",│   │
│   │                    │  "intensity":"NORMAL","speed":1.0}}               │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ sentiment          │ {"type":"sentiment","value":"BULLISH"}            │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ intensity          │ {"type":"intensity","value":"AGGRESSIVE"}         │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ spread             │ {"type":"spread","value":"0.15"}                  │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ speed              │ {"type":"speed","value":"1.5"}                    │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ pause              │ {"type":"pause","value":"true"}                   │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ reset              │ {"type":"reset","value":"true"}                   │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ newsShock          │ {"type":"newsShock","value":"true"}               │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ getCandles         │ {"type":"getCandles","timeframe":60}              │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ ping               │ {"type":"ping","timestamp":1705574400000}         │   │
│   └────────────────────┴───────────────────────────────────────────────────┘   │
│                                                                                 │
│                                                                                 │
│   SERVER → CLIENT Messages:                                                     │
│   ─────────────────────────                                                     │
│                                                                                 │
│   ┌────────────────────┬───────────────────────────────────────────────────┐   │
│   │ Type               │ Description                                       │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ started            │ {"type":"started"} - Confirms simulation began    │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ tick               │ Batched update every ~100ms (see Data Flow)       │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ candleHistory      │ {"type":"candleHistory","data":{"timeframe":60,   │   │
│   │                    │  "candles":[...],"current":{...}}}                │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ candleReset        │ {"type":"candleReset"} - Clear caches on reset    │   │
│   ├────────────────────┼───────────────────────────────────────────────────┤   │
│   │ pong               │ {"type":"pong","timestamp":1705574400000}         │   │
│   └────────────────────┴───────────────────────────────────────────────────┘   │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 10. Component Interactions

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                      COMPONENT INTERACTION DIAGRAM                              │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  Price Engine ──► Candle Manager ──► Order Book Generation                      │
│       │                │                      │                                 │
│       │                │                      │                                 │
│       ▼                ▼                      ▼                                 │
│  ┌─────────┐      ┌─────────┐           ┌─────────┐                            │
│  │ Current │      │ OHLCV   │           │ Bids/   │                            │
│  │ Price   │      │ Candles │           │ Asks    │                            │
│  └────┬────┘      └────┬────┘           └────┬────┘                            │
│       │                │                      │                                 │
│       └────────────────┼──────────────────────┘                                │
│                        │                                                        │
│                        ▼                                                        │
│              ┌─────────────────┐                                               │
│              │   Tick Message  │                                               │
│              │  (JSON Bundle)  │                                               │
│              └────────┬────────┘                                               │
│                       │                                                         │
│                       │ WebSocket                                               │
│                       ▼                                                         │
│              ┌─────────────────┐                                               │
│              │  useWebSocket   │                                               │
│              │     Hook        │                                               │
│              └────────┬────────┘                                               │
│                       │                                                         │
│       ┌───────────────┼───────────────┬───────────────┐                        │
│       │               │               │               │                        │
│       ▼               ▼               ▼               ▼                        │
│  ┌─────────┐    ┌──────────┐   ┌──────────┐   ┌───────────┐                   │
│  │OrderBook│    │Candlestick│   │  Stats   │   │Trade      │                   │
│  │Component│    │  Chart   │   │  Panel   │   │Ticker     │                   │
│  └─────────┘    └──────────┘   └──────────┘   └───────────┘                   │
│                                                                                 │
│                                                                                 │
│  User Interaction Flow:                                                         │
│  ──────────────────────                                                         │
│                                                                                 │
│  ┌────────────┐       ┌─────────────┐       ┌──────────────┐                   │
│  │ Control    │──────►│ useWebSocket│──────►│ WebSocket    │                   │
│  │ Panel      │ send  │   .send()   │  WS   │ Server       │                   │
│  └────────────┘       └─────────────┘       └──────┬───────┘                   │
│                                                     │                           │
│                                                     │ processMessage            │
│                                                     ▼                           │
│                                              ┌──────────────┐                   │
│                                              │ SessionState │                   │
│                                              │ .setSentiment│                   │
│                                              └──────────────┘                   │
│                                                     │                           │
│                                                     │ Next tick                 │
│                                                     ▼                           │
│                                              ┌──────────────┐                   │
│                                              │ Updated data │                   │
│                                              │ flows back   │                   │
│                                              └──────────────┘                   │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 11. Why NOT Lock-Free?

This project uses **mutex-based synchronization** instead of lock-free data structures:

### Reasons for Mutex-Based Approach:

1. **Simplicity**
   - `std::mutex` + `condition_variable` is well-understood
   - Easy to reason about correctness
   - Standard library support, no external dependencies

2. **Low Contention**
   - Only 2-3 threads access shared data
   - Lock hold times are microseconds (just push/pop)
   - No high-frequency trading latency requirements

3. **Blocking is Acceptable**
   - Consumer can wait (uses `condition_variable`)
   - Producer never blocks long (queue rarely full)
   - 100ms tick interval >> any lock wait time

4. **Lock-Free Complexity**
   - ABA problem handling
   - Memory ordering concerns (`memory_order_acquire/release`)
   - Hazard pointers or epoch-based reclamation
   - Much harder to debug

### When to Use Lock-Free:

```
Lock-Free makes sense when:
  ✓ Millions of operations per second
  ✓ Many threads competing for same resource
  ✓ Latency measured in nanoseconds matters
  ✓ Can't afford thread blocking ever

This project:
  ✗ ~10 operations per second per session
  ✗ 2-5 threads total
  ✗ 100ms latency is fine
  ✗ Blocking during wait is expected
```

### Our Synchronization Is Good Enough:

```cpp
// OrderQueue: ~1000ns per push/pop
// Lock overhead: ~50ns for uncontested mutex
// Total: ~1050ns = 0.001ms

// We send 10 ticks/second = 100ms between sends
// Lock overhead is 0.001% of tick interval

// Conclusion: Mutex overhead is negligible
```

---

## 12. Performance Considerations

### Current Performance Profile

| Metric | Value | Notes |
|--------|-------|-------|
| Tick rate | 10/second | 100ms interval |
| Message size | ~2KB | Typical tick JSON |
| Memory/session | ~50KB | OrderBook + Candles + State |
| CPU usage | <5% | Per session |
| Max sessions | ~100 | Limited by memory |

### Bottlenecks & Optimizations

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         PERFORMANCE ANALYSIS                                    │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                 │
│  Potential Bottleneck: JSON Serialization                                       │
│  ─────────────────────────────────────────                                      │
│  • Currently using std::ostringstream                                           │
│  • Creates new string every tick                                                │
│  • Could optimize with pre-allocated buffers                                    │
│                                                                                 │
│  Current (works fine):                                                          │
│  std::ostringstream ss;                                                         │
│  ss << R"({"type":"tick","data":{...}})";                                       │
│  return ss.str();  // ~2KB allocation per tick                                  │
│                                                                                 │
│  Optimized (if needed):                                                         │
│  char buffer[4096];                                                             │
│  snprintf(buffer, sizeof(buffer), "{\"type\":\"tick\",...}", ...);              │
│  // Zero allocations                                                            │
│                                                                                 │
│                                                                                 │
│  Potential Bottleneck: Session Iteration                                        │
│  ─────────────────────────────────────────                                      │
│  • getAllSessions() copies vector of pointers                                   │
│  • Lock held during iteration                                                   │
│  • Fine for <100 sessions                                                       │
│                                                                                 │
│  Current:                                                                       │
│  for (auto* session : wsServer->getAllSessions()) {  // Lock acquired           │
│      updateSession(session);                         // Lock held               │
│  }                                                   // Lock released           │
│                                                                                 │
│  Better (if scaling to 1000s):                                                  │
│  auto sessions = wsServer->getAllSessions();  // Copy under lock                │
│  for (auto* session : sessions) {             // No lock needed                 │
│      updateSession(session);                                                    │
│  }                                                                              │
│                                                                                 │
│                                                                                 │
│  NOT a Bottleneck: libwebsockets                                                │
│  ─────────────────────────────────                                              │
│  • Highly optimized C library                                                   │
│  • Handles thousands of connections                                             │
│  • We're nowhere near its limits                                                │
│                                                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## Summary

| Aspect | Implementation |
|--------|----------------|
| **Threading** | 5 threads: Generator, Processor, WebSocket, Keyboard, Display |
| **Synchronization** | Mutex + condition_variable (NOT lock-free) |
| **Sessions** | Per-client SessionState with isolated components |
| **Communication** | WebSocket with `lws-minimal` protocol |
| **Data Flow** | Server-authoritative, 10 ticks/second |
| **State** | Server owns all state, client is view-only |

**The architecture prioritizes:**
- ✅ Correctness over performance
- ✅ Simplicity over complexity
- ✅ Session isolation for multi-user support
- ✅ Real-time updates with acceptable latency

---

*End of Architecture Document*
