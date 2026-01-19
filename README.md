# MarketPulse - Real-Time Order Book Visualizer

A real-time market simulation featuring a C++ WebSocket backend with a React/TypeScript frontend. Watch order books fill and drain, prices move based on market sentiment, and trades execute in real-time.

![MarketPulse](https://img.shields.io/badge/MarketPulse-v1.0-green) ![C++17](https://img.shields.io/badge/C++-17-blue) ![React](https://img.shields.io/badge/React-18-61dafb) ![TypeScript](https://img.shields.io/badge/TypeScript-5-3178c6)

## Features

- 📊 **Real-time Order Book** - Live bid/ask visualization with depth
- 📈 **Candlestick Charts** - Multiple timeframes (1s, 5s, 30s, 1m, 5m)
- ⚡ **Market Sentiments** - Bullish, Bearish, Volatile, Sideways, Choppy, Neutral
- 🎚️ **Intensity Levels** - Mild to Extreme price movement control
- 📰 **News Shocks** - Random market-moving events
- 🔄 **Live Trade Ticker** - Real-time trade feed
- 📉 **Market Stats** - VWAP, high/low, volume tracking

## Prerequisites

### For C++ Backend

- **MSYS2/MinGW-w64** (Windows) or **GCC 9+** (Linux/Mac)
- **CMake 3.16+**
- **libwebsockets 4.4+**

### For React Frontend

- **Node.js 18+**
- **npm** or **yarn**

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/order-book-visualizer.git
cd order-book-visualizer
```

### 2. Build the C++ Backend

**Windows (MSYS2/MinGW):**

```powershell
# Add MinGW to PATH (run in PowerShell)
$env:Path = "C:\msys64\mingw64\bin;$env:Path"

# Create build directory and build
cd order-book-visualizer
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build . --target orderbook
```

**Linux/Mac:**

```bash
mkdir build && cd build
cmake ..
make orderbook
```

The executable will be at `build/bin/orderbook.exe` (Windows) or `build/bin/orderbook` (Linux/Mac).

### 3. Install Frontend Dependencies

```bash
cd frontend
npm install
```

### 4. Run the Application

**Terminal 1 - Start C++ Backend:**

```powershell
# Windows
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cd build\bin
.\orderbook.exe --headless --auto-start
```

```bash
# Linux/Mac
./build/bin/orderbook --headless --auto-start
```

**Terminal 2 - Start React Frontend:**

```bash
cd frontend
npm run dev
```

### 5. Open in Browser

Navigate to **http://localhost:5173**

## Configuration

### Landing Page Options

| Option | Range | Default | Description |
|--------|-------|---------|-------------|
| Symbol | Any | DEMO | Stock ticker symbol |
| Price | $100-$500 | $100 | Starting price |
| Spread | $0.05-$0.25 | $0.05 | Bid-ask spread |
| Sentiment | 6 options | Neutral | Market direction bias |
| Intensity | 5 levels | Normal | Price movement magnitude |
| Speed | 0.25x-2x | 1x | Simulation speed |

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
  -h, --help              Show help
```

### Example Commands

```bash
# Default headless mode (for web UI)
./orderbook.exe --headless --auto-start

# Interactive setup
./orderbook.exe -i

# Custom configuration
./orderbook.exe -p 250 -s AAPL --sentiment bullish --intensity aggressive

# Console mode with visualization
./orderbook.exe -p 150 --sentiment volatile
```

## Market Sentiments

| Sentiment | Description | Price Behavior |
|-----------|-------------|----------------|
| 🐂 Bullish | Upward bias | ~62% chance of price increase |
| 🐻 Bearish | Downward bias | ~62% chance of price decrease |
| ⚡ Volatile | Large swings | High volatility, sudden reversals |
| ↔️ Sideways | Range-bound | Mean reversion around anchor |
| 🌊 Choppy | Erratic | Unpredictable, frequent direction changes |
| ⚖️ Neutral | Balanced | 50/50 up/down, baseline volatility |

## Intensity Levels

| Level | Multiplier | Effect |
|-------|------------|--------|
| Mild | 0.4x | Minimal price movement |
| Moderate | 0.7x | Reduced volatility |
| Normal | 0.85x | Standard behavior |
| Aggressive | 1.0x | Enhanced movement |
| Extreme | 1.25x | Maximum volatility |

## Architecture

```
┌─────────────────┐         WebSocket (8080)        ┌─────────────────┐
│   C++ Backend   │◄──────────────────────────────►│  React Frontend │
│                 │                                 │                 │
│ • Order Book    │         JSON Messages           │ • Charts        │
│ • Price Engine  │  ◄─────────────────────────►   │ • Order Book    │
│ • Trade Gen     │                                 │ • Controls      │
│ • Candle Mgr    │                                 │ • Stats         │
└─────────────────┘                                 └─────────────────┘
     Port 8080                                           Port 5173
```

## Project Structure

```
order-book-visualizer/
├── src/                    # C++ source files
├── include/                # C++ headers
├── build/                  # CMake build output
│   └── bin/               # Compiled executable
├── frontend/              # React application
│   ├── src/
│   │   ├── components/    # UI components
│   │   ├── hooks/         # Custom React hooks
│   │   ├── pages/         # Page components
│   │   └── types.ts       # TypeScript types
│   └── package.json
├── CMakeLists.txt         # CMake configuration
├── ARCHITECTURE.md        # Detailed architecture docs
├── KNOWLEDGE.md           # Project knowledge base
└── README.md              # This file
```

## Troubleshooting

### Backend won't start

1. Ensure MinGW is in PATH: `$env:Path = "C:\msys64\mingw64\bin;$env:Path"`
2. Check if port 8080 is available
3. Rebuild: `cmake --build . --target orderbook`

### Frontend can't connect

1. Verify backend is running on port 8080
2. Check browser console for WebSocket errors
3. Ensure no CORS issues (both on localhost)

### Price not moving

- At low prices, movements are percentage-based
- Try increasing intensity or using volatile sentiment

### Build errors

```bash
# Clean rebuild
cd build
rm -rf *
cmake .. -G "MinGW Makefiles"  # Windows
cmake ..                        # Linux/Mac
cmake --build . --target orderbook
```

## Development

### Rebuild Backend After Changes

```powershell
$env:Path = "C:\msys64\mingw64\bin;$env:Path"
cd build
cmake --build . --target orderbook
```

### Frontend Hot Reload

The frontend uses Vite with hot module replacement. Changes are reflected immediately.

## License

MIT License - See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

---

**Built with ❤️ using C++17, libwebsockets, React 18, and TypeScript**
