// ============================================================================
// MAIN.CPP - Order Book Visualizer Entry Point
// ============================================================================
// Realistic market simulation with configurable:
// - Base price, Stock symbol, Spread, Sentiment, Intensity
// - Real-time keyboard controls for dynamic adjustments
// - Pause/Resume, Speed control, Reset functionality
// - WebSocket server for frontend integration
// ============================================================================

#include "Common.h"
#include "Order.h"
#include "OrderBook.h"
#include "MatchingEngine.h"
#include "OrderQueue.h"
#include "Visualizer.h"
#include "MarketSentiment.h"
#include "NewsShock.h"
#include "PriceEngine.h"
#include "CandleManager.h"
#include "SessionState.h"

#ifdef WEBSOCKET_ENABLED
#include "WebSocketServer.h"
#endif

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <csignal>
#include <iomanip>
#include <mutex>
#include <ctime>
#include <sstream>

#ifdef _WIN32
#include <conio.h>  // For _kbhit() and _getch() on Windows
#endif

using namespace orderbook;

// ============================================================================
// SIMULATION CONFIGURATION
// ============================================================================

struct SimulationConfig {
    double basePrice = 100.0;       // Starting price ($100 - $500)
    std::string stockSymbol = "DEMO"; // Stock ticker symbol
    double spread = 0.05;           // Initial spread ($0.05 - $0.25)
    Sentiment sentiment = Sentiment::NEUTRAL;
    Intensity intensity = Intensity::NORMAL;
    double speedMultiplier = 1.0;   // Order generation speed (0.25x - 4x)
    bool autoStart = false;         // Skip "press any key" prompt
    bool waitForWebSocket = false;  // Wait for WebSocket start command
    bool headless = false;          // No terminal visualization, just logs
    bool debug = false;             // Enable verbose debug logging
    
    // Validate and clamp values
    void validate() {
        basePrice = std::max(100.0, std::min(500.0, basePrice));
        spread = std::max(0.05, std::min(0.25, spread));
        speedMultiplier = std::max(0.25, std::min(4.0, speedMultiplier));
        // Round to tick size
        basePrice = std::round(basePrice / 0.05) * 0.05;
        spread = std::round(spread / 0.05) * 0.05;
    }
};

// Global debug flag for easy access
bool g_debug = false;// ============================================================================
// GLOBAL STATE
// ============================================================================

std::atomic<bool> g_running{true};
std::atomic<bool> g_paused{false};
std::atomic<double> g_speedMultiplier{1.0};
std::atomic<bool> g_wsStartReceived{false};  // WebSocket start signal
MarketSentimentController g_sentimentController;
SimulationConfig g_config;

// News Shock controller
orderbook::NewsShockController g_newsShockController;

// Price engine (for trend/pullback logic like Node.js)
orderbook::PriceEngine g_priceEngine;

// Candle manager (for multi-timeframe aggregation like Node.js)
orderbook::CandleManager g_candleManager;

// WebSocket server
#ifdef WEBSOCKET_ENABLED
WebSocketServer* g_wsServer = nullptr;
#endif

// Price tracking for stats
std::atomic<double> g_openPrice{100.0};
std::atomic<double> g_highPrice{100.0};
std::atomic<double> g_lowPrice{100.0};
std::atomic<double> g_currentPrice{100.0};

// For price logging
std::mutex g_logMutex;
std::ofstream g_priceLog;
Sentiment g_lastLoggedSentiment = Sentiment::NEUTRAL;
Intensity g_lastLoggedIntensity = Intensity::NORMAL;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n\nShutting down gracefully...\n";
        g_running = false;
        
        #ifdef WEBSOCKET_ENABLED
        if (g_wsServer) {
            // Print final stats
            g_wsServer->printStats();
            
            // Stop the server (cleans up all sessions)
            g_wsServer->stop();
            std::cout << "WebSocket server stopped, all sessions cleaned up.\n";
        }
        #endif
    }
}

// ============================================================================
// PRICE LOGGER
// ============================================================================
// Logs prices to "prices.txt" with sentiment/intensity changes

void initPriceLog() {
    g_priceLog.open("prices.txt", std::ios::app);
    if (g_priceLog.is_open()) {
        // Write header if file is empty/new
        g_priceLog.seekp(0, std::ios::end);
        if (g_priceLog.tellp() == 0) {
            g_priceLog << "# Order Book Visualizer - Price Log\n";
            g_priceLog << "# Format: TIMESTAMP, PRICE, SENTIMENT, INTENSITY, CHANGE_TYPE\n";
            g_priceLog << "# CHANGE_TYPE: TRADE, SENTIMENT_CHANGE, INTENSITY_CHANGE, BOTH_CHANGE\n";
            g_priceLog << "# ============================================================\n";
        }
        g_priceLog << "\n# === NEW SESSION: " << std::time(nullptr) << " ===\n";
        g_priceLog.flush();
    }
}

void logPrice(double price, const std::string& changeType = "TRADE") {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_priceLog.is_open()) return;
    
    Sentiment currentSentiment = g_sentimentController.getSentiment();
    Intensity currentIntensity = g_sentimentController.getIntensity();
    
    // Get timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    // Format: HH:MM:SS.mmm
    std::tm* tm_now = std::localtime(&time_t_now);
    
    g_priceLog << std::setfill('0') << std::setw(2) << tm_now->tm_hour << ":"
               << std::setw(2) << tm_now->tm_min << ":"
               << std::setw(2) << tm_now->tm_sec << "."
               << std::setw(3) << ms.count() << ", ";
    
    // Price
    g_priceLog << std::fixed << std::setprecision(2) << price << ", ";
    
    // Sentiment name
    g_priceLog << MarketSentimentController::getSentimentName(currentSentiment) << ", ";
    
    // Intensity name
    g_priceLog << MarketSentimentController::getIntensityName(currentIntensity) << ", ";
    
    // Change type
    g_priceLog << changeType << "\n";
    g_priceLog.flush();
}

void logSentimentChange(double currentPrice) {
    Sentiment currentSentiment = g_sentimentController.getSentiment();
    Intensity currentIntensity = g_sentimentController.getIntensity();
    
    bool sentimentChanged = (currentSentiment != g_lastLoggedSentiment);
    bool intensityChanged = (currentIntensity != g_lastLoggedIntensity);
    
    if (sentimentChanged || intensityChanged) {
        std::string changeType;
        if (sentimentChanged && intensityChanged) {
            changeType = "BOTH_CHANGE";
        } else if (sentimentChanged) {
            changeType = "SENTIMENT_CHANGE";
        } else {
            changeType = "INTENSITY_CHANGE";
        }
        
        logPrice(currentPrice, changeType);
        g_lastLoggedSentiment = currentSentiment;
        g_lastLoggedIntensity = currentIntensity;
    }
}

void closePriceLog() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_priceLog.is_open()) {
        g_priceLog << "# === SESSION END ===\n";
        g_priceLog.close();
    }
}

// ============================================================================
// ORDER GENERATOR (Producer Thread)
// ============================================================================
// Generates limit orders (add liquidity) and market orders (take liquidity)

// Global generator shared between threads
SentimentOrderGenerator* g_generator = nullptr;
std::mutex g_generatorMutex;

void orderGenerator(OrderQueue& queue, std::atomic<OrderId>& nextOrderId,
                    const OrderBook& orderBook) {
    
    while (g_running) {
        // Check if paused
        if (g_paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        // Update generator with current order book state
        {
            std::lock_guard<std::mutex> lock(g_generatorMutex);
            if (g_generator) {
                auto bestBid = orderBook.getBestBid();
                auto bestAsk = orderBook.getBestAsk();
                if (bestBid && bestAsk) {
                    g_generator->updateFromOrderBook(*bestBid, *bestAsk);
                }
            }
        }
        
        // Generate order
        SentimentOrderGenerator::GeneratedOrder genOrder;
        {
            std::lock_guard<std::mutex> lock(g_generatorMutex);
            if (g_generator) {
                genOrder = g_generator->generateOrder();
            } else {
                continue;
            }
        }
        
        OrderId id = nextOrderId++;
        OrderType type = genOrder.isMarketOrder ? OrderType::MARKET : OrderType::LIMIT;
        
        // Create and queue the order
        Order order(id, genOrder.side, type, genOrder.price, genOrder.quantity);
        queue.push(order);
        
        // Delay based on sentiment AND speed multiplier
        int delay;
        {
            std::lock_guard<std::mutex> lock(g_generatorMutex);
            delay = g_generator ? g_generator->getNextDelay() : 50;
        }
        // Apply speed multiplier (higher = faster = shorter delay)
        delay = static_cast<int>(delay / g_speedMultiplier.load());
        delay = std::max(5, delay);  // Minimum 5ms
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
}

// ============================================================================
// ORDER PROCESSOR (Consumer Thread)
// ============================================================================
// Processes orders and feeds trade executions back to the generator

void orderProcessor(OrderQueue& queue, MatchingEngine& engine, 
                    Visualizer& visualizer, std::atomic<size_t>& processedCount,
                    std::atomic<size_t>& marketOrderCount,
                    std::atomic<size_t>& limitOrderCount,
                    OrderBook& /*orderBook*/) {
    
    int tradeCounter = 0;
    
    while (g_running) {
        auto orderOpt = queue.popWithTimeout(100);
        
        if (orderOpt) {
            Order& order = *orderOpt;
            
            // Track order types
            if (order.getType() == OrderType::MARKET) {
                marketOrderCount++;
            } else {
                limitOrderCount++;
            }
            
            // Process the order through the matching engine
            auto trades = engine.processOrder(order);
            
            // For each trade, update the generator's price and log
            for (const auto& trade : trades) {
                // Record trade for visualization
                visualizer.addTrade(trade.price, trade.quantity, order.getSide());
                
                // Update price tracking
                g_currentPrice = trade.price;
                double high = g_highPrice.load();
                while (trade.price > high && !g_highPrice.compare_exchange_weak(high, trade.price)) {}
                double low = g_lowPrice.load();
                while (trade.price < low && !g_lowPrice.compare_exchange_weak(low, trade.price)) {}
                
                // CRITICAL: Feed trade back to generator - this drives price movement!
                {
                    std::lock_guard<std::mutex> lock(g_generatorMutex);
                    if (g_generator) {
                        g_generator->onTradeExecuted(trade.price, order.getSide());
                    }
                }
                
                // NOTE: Trades are now sent per-session in displayUpdater, not broadcast
                // Each session generates its own trades based on its own price

                // Log every 10th trade to avoid huge file
                tradeCounter++;
                if (tradeCounter % 10 == 0) {
                    logPrice(trade.price, "TRADE");
                }
            }
            
            processedCount++;
        }
    }
}

// ============================================================================
// KEYBOARD INPUT HANDLER (Input Thread)
// ============================================================================
// Handles keyboard input to change market sentiment and intensity in real-time

void keyboardHandler() {
#ifdef _WIN32
    while (g_running) {
        if (_kbhit()) {
            char c = _getch();
            switch (c) {
                // Sentiment controls (1-6)
                case '1':
                    g_sentimentController.setSentiment(Sentiment::BULLISH);
                    break;
                case '2':
                    g_sentimentController.setSentiment(Sentiment::BEARISH);
                    break;
                case '3':
                    g_sentimentController.setSentiment(Sentiment::VOLATILE);
                    break;
                case '4':
                    g_sentimentController.setSentiment(Sentiment::CALM);
                    break;
                case '5':
                    g_sentimentController.setSentiment(Sentiment::CHOPPY);
                    break;
                case '6':
                    g_sentimentController.setSentiment(Sentiment::NEUTRAL);
                    break;
                    
                // Intensity controls (F1-F5 or shifted numbers)
                case '!':  // Shift+1 = Mild
                    g_sentimentController.setIntensity(Intensity::MILD);
                    break;
                case '@':  // Shift+2 = Moderate
                    g_sentimentController.setIntensity(Intensity::MODERATE);
                    break;
                case '#':  // Shift+3 = Normal
                    g_sentimentController.setIntensity(Intensity::NORMAL);
                    break;
                case '$':  // Shift+4 = Aggressive
                    g_sentimentController.setIntensity(Intensity::AGGRESSIVE);
                    break;
                case '%':  // Shift+5 = Extreme
                    g_sentimentController.setIntensity(Intensity::EXTREME);
                    break;
                    
                // Also support letter keys for intensity
                case 'm': case 'M':  // Mild
                    g_sentimentController.setIntensity(Intensity::MILD);
                    break;
                case 'n': case 'N':  // Normal
                    g_sentimentController.setIntensity(Intensity::NORMAL);
                    break;
                case 'a': case 'A':  // Aggressive
                    g_sentimentController.setIntensity(Intensity::AGGRESSIVE);
                    break;
                case 'x': case 'X':  // eXtreme
                    g_sentimentController.setIntensity(Intensity::EXTREME);
                    break;
                    
                case ' ':  // Spacebar cycles sentiment
                    g_sentimentController.nextSentiment();
                    break;
                case '\t':  // Tab cycles intensity
                    g_sentimentController.nextIntensity();
                    break;
                
                // Spread controls (+ / -)
                case '+':
                case '=':
                    g_sentimentController.increaseSpread();
                    break;
                case '-':
                case '_':
                    g_sentimentController.decreaseSpread();
                    break;
                
                // Pause/Resume (P)
                case 'p':
                case 'P':
                    g_paused = !g_paused.load();
                    break;
                
                // Speed controls (F = Faster, S = Slower)
                case 'f':
                case 'F':
                    {
                        double speed = g_speedMultiplier.load();
                        if (speed < 4.0) {
                            g_speedMultiplier = std::min(4.0, speed * 2.0);
                        }
                    }
                    break;
                case 's':
                case 'S':
                    {
                        double speed = g_speedMultiplier.load();
                        if (speed > 0.25) {
                            g_speedMultiplier = std::max(0.25, speed / 2.0);
                        }
                    }
                    break;
                    
                case 'q':
                case 'Q':
                case 27:   // ESC key
                    g_running = false;
                    break;
                    
                // Show stats (I = Info)
                case 'i':
                case 'I':
                    #ifdef WEBSOCKET_ENABLED
                    if (g_wsServer) {
                        g_wsServer->printStats();
                    }
                    #endif
                    break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
#endif
}

// ============================================================================
// DISPLAY UPDATER (Visualization Thread)
// ============================================================================
// This updates the display periodically and broadcasts WebSocket data

void displayUpdater(Visualizer& visualizer, std::atomic<size_t>& processedCount,
                    MatchingEngine& engine,
                    std::atomic<size_t>& marketOrderCount,
                    std::atomic<size_t>& limitOrderCount,
                    OrderBook& /*orderBook*/) {
    while (g_running) {
        // Only render terminal UI if not headless
        if (!g_config.headless) {
            visualizer.render(10);  // Show top 10 levels
        }
        
        // Check for sentiment/intensity changes and log them
        double lastTradePrice = g_currentPrice.load();
        {
            std::lock_guard<std::mutex> lock(g_generatorMutex);
            if (g_generator) {
                lastTradePrice = g_generator->getLastTradePrice();
            }
        }
        logSentimentChange(lastTradePrice);
        
        // Show current sentiment, intensity and stats
        Sentiment s = g_sentimentController.getSentiment();
        std::string color = MarketSentimentController::getSentimentColor(s);
        double spread = g_sentimentController.getSpread();
        
        size_t mktOrders = marketOrderCount.load();
        size_t limOrders = limitOrderCount.load();
        size_t totalOrders = mktOrders + limOrders;
        int mktPct = (totalOrders > 0) ? static_cast<int>(100.0 * mktOrders / totalOrders) : 0;
        
        // Get paused state and speed
        bool paused = g_paused.load();
        double speed = g_speedMultiplier.load();
        
        // Check news shock expiration
        g_newsShockController.checkExpiration();
        
        // Broadcast to WebSocket clients - EACH SESSION GETS ITS OWN DATA
        #ifdef WEBSOCKET_ENABLED
        if (g_wsServer && g_wsServer->getConnectionCount() > 0) {
            // Get timestamp
            auto now = std::chrono::system_clock::now();
            int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            
            // Process each session independently
            auto clientIds = g_wsServer->getClientIds();
            for (uint32_t clientId : clientIds) {
                SessionState* session = g_wsServer->getSession(clientId);
                if (!session || !session->isRunning()) continue;
                
                // Get session-specific values
                double sessionSpread = session->getSpread();
                double sessionSpeed = session->getSpeed();
                Sentiment sessionSentiment = session->getSentiment();
                Intensity sessionIntensity = session->getIntensity();
                bool isPaused = session->isPaused();
                
                // Per-session timing: check if enough time has passed based on speed
                // Base interval = 100ms, effective interval = 100ms / speed
                // At 2x speed, we update every 50ms; at 0.5x, every 200ms
                int64_t effectiveInterval = static_cast<int64_t>(100.0 / sessionSpeed);
                int64_t lastUpdate = session->getLastUpdateTime();
                if (lastUpdate > 0 && (timestamp - lastUpdate) < effectiveInterval) {
                    continue;  // Skip this session, not time to update yet
                }
                session->setLastUpdateTime(timestamp);
                
                // Get sentiment/intensity strings for price engine
                std::string sentimentStr = MarketSentimentController::getSentimentNameSimple(sessionSentiment);
                std::string intensityStr = MarketSentimentController::getIntensityNameSimple(sessionIntensity);
                
                double sessionPrice = session->getCurrentPrice();
                int tickVolume = 0;
                std::vector<orderbook::CompletedCandle> completedCandles;
                TradeData* tradePtr = nullptr;  // No trade by default (or when paused)
                TradeData tradeData;
                
                // Only update simulation state if NOT paused
                if (!isPaused) {
                    // Check session's news shock
                    session->getNewsShockController().checkExpiration();
                    
                    // Generate price using session's price engine
                    PriceEngine& priceEngine = session->getPriceEngine();
                    double currentPrice = session->getCurrentPrice();
                    bool newsShockEnabled = session->getNewsShockController().isEnabled();
                    auto priceResult = priceEngine.calculateNextPrice(
                        currentPrice,
                        sentimentStr,
                        intensityStr,
                        newsShockEnabled
                    );
                    
                    // Log significant price changes (debug mode only)
                    if (g_debug) {
                        double priceChange = priceResult.newPrice - currentPrice;
                        double pctChange = (priceChange / currentPrice) * 100.0;
                        if (std::abs(pctChange) > 0.5) {  // Log moves > 0.5%
                            std::cout << "[Price] Session " << clientId 
                                      << " " << sentimentStr << "/" << intensityStr
                                      << " NewsShock=" << (newsShockEnabled ? "ON" : "OFF")
                                      << " $" << std::fixed << std::setprecision(2) << currentPrice 
                                      << " -> $" << priceResult.newPrice 
                                      << " (" << (priceChange >= 0 ? "+" : "") << std::setprecision(2) << pctChange << "%)";
                            if (priceResult.shockApplied) {
                                std::cout << " [SHOCK: " << priceResult.shockType << "]";
                            }
                            std::cout << "\n";
                        }
                    }
                    
                    session->setCurrentPrice(priceResult.newPrice);
                    sessionPrice = session->getCurrentPrice();
                    
                    // Generate tick volume and simulate trading activity
                    tickVolume = 10 + (rand() % 40);
                    session->addVolume(tickVolume);
                    session->addOrders(1 + rand() % 3);
                    
                    // Simulate trades (roughly 1 trade per 2-3 ticks)
                    if (rand() % 3 == 0) {
                        tradeData = session->generateTrade(sessionPrice, timestamp);
                        tradePtr = &tradeData;
                        
                        // Log every 10th trade per session to prices.txt
                        static std::map<uint32_t, int> sessionTradeCounters;
                        sessionTradeCounters[clientId]++;
                        if (sessionTradeCounters[clientId] % 10 == 0) {
                            logPrice(tradeData.price, "TRADE");
                        }
                        
                        if (g_debug) {
                            std::cout << "[Trade] Session " << clientId << " Price: $" << std::fixed << std::setprecision(2) 
                                      << sessionPrice << " -> Trade: $" << tradeData.price << " (" << tradeData.side << ")\n";
                        }
                    }
                    
                    // Simulate market/limit order ratio (~20% market, 80% limit)
                    if (rand() % 5 == 0) {
                        session->addMarketOrder();
                    } else {
                        session->addLimitOrder();
                    }
                    
                    // Update candles for this session
                    CandleManager& candleManager = session->getCandleManager();
                    completedCandles = candleManager.updateCandles(sessionPrice, tickVolume, timestamp);
                    
                    // Regenerate order book only when NOT paused
                    SentimentOrderGenerator generator(session->getSentimentController());
                    generator.regenerateOrderBook(session->getOrderBook(), sessionPrice, sessionSpread);
                }
                
                // Always get current order book and candles (to show frozen state when paused)
                OrderBook& sessionOrderBook = session->getOrderBook();
                auto currentCandles = session->getCandleManager().getCurrentCandles();
                
                // Build stats JSON for this session
                std::string statsJson = JsonBuilder::statsToJson(
                    session->getSymbol(),
                    sessionPrice,
                    session->getOpenPrice(),
                    session->getHighPrice(),
                    session->getLowPrice(),
                    session->getTotalOrders(),
                    session->getTotalTrades(),
                    session->getTotalVolume(),
                    session->getMarketOrderPct(),
                    sentimentStr,
                    intensityStr,
                    sessionSpread,
                    sessionSpeed,
                    session->isPaused(),
                    session->getNewsShockController().isEnabled(),
                    session->getNewsShockController().isInCooldown(),
                    session->getNewsShockController().getCooldownRemaining(),
                    session->getNewsShockController().getActiveRemaining()
                );
                
                // Build and send batched tick message to THIS client only
                std::string tickJson = JsonBuilder::tickToJson(
                    sessionOrderBook,
                    statsJson,
                    sessionPrice,
                    tickVolume,
                    timestamp,
                    tradePtr,  // Pass trade pointer (nullptr when paused)
                    currentCandles,
                    completedCandles
                );
                g_wsServer->sendToClient(clientId, tickJson);
            }
        }
        #endif
        
        // Only print status line if not headless
        if (!g_config.headless) {
            std::cout << "\n  " << g_config.stockSymbol << " @ $" << std::fixed << std::setprecision(2) << lastTradePrice;
            if (paused) {
                std::cout << "  \033[93;1m[PAUSED]\033[0m";
            }
            #ifdef WEBSOCKET_ENABLED
            if (g_wsServer) {
                std::cout << "  \033[96m[WS:" << g_wsServer->getConnectionCount() << "]\033[0m";
            }
            #endif
            std::cout << "  Speed: " << speed << "x";
            std::cout << "\n  Orders: " << processedCount.load();
            std::cout << " (\033[91mMKT:" << mktPct << "%\033[0m";
            std::cout << " \033[92mLIM:" << (100-mktPct) << "%\033[0m)";
            std::cout << "  |  Trades: " << engine.getTradeCount();
            std::cout << "  |  Vol: " << engine.getTotalVolume();
            std::cout << "\n  " << "\033[93mSPREAD: $" << std::fixed << std::setprecision(2) << spread << "\033[0m";
            std::cout << "  |  " << color << g_sentimentController.getMarketConditionString() << "\033[0m";
            std::cout << "\n  [1-6]=Sentiment [M/N/A/X]=Intensity [+/-]=Spread [P]=Pause [F/S]=Speed [Q]=Quit\n";
        }
        
        // 50ms base tick rate - allows per-session speed control up to 2x
        // Each session tracks its own timing based on speed setting
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ============================================================================
// PRINT USAGE / HELP
// ============================================================================

void printUsage(const char* programName) {
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "   ORDER BOOK VISUALIZER with Market Sentiment & Spread\n";
    std::cout << "================================================================\n";
    std::cout << "\nUsage: " << programName << " [options]\n\n";
    std::cout << "OPTIONS:\n";
    std::cout << "  -i, --interactive       Interactive setup (prompts for all options)\n";
    std::cout << "  -p, --price <value>     Base price ($100 - $500, default: $100)\n";
    std::cout << "  -s, --symbol <name>     Stock symbol (default: DEMO)\n";
    std::cout << "  --spread <value>        Initial spread ($0.05 - $0.25, step: $0.05)\n";
    std::cout << "  --sentiment <type>      Initial sentiment (see below)\n";
    std::cout << "  --intensity <level>     Initial intensity (see below)\n";
    std::cout << "  -a, --auto-start        Skip 'press any key' prompt\n";
    std::cout << "  -w, --wait-for-ws       Wait for WebSocket start command\n";
    std::cout << "  --headless              No terminal UI, just logs (for WebSocket mode)\n";
    std::cout << "  -d, --debug             Enable verbose debug logging\n";
    std::cout << "  -h, --help              Show this help\n";
    std::cout << "\nSENTIMENTS:\n";
    std::cout << "  bullish  (bull, up)     - Prices trending UP       [^^]\n";
    std::cout << "  bearish  (bear, down)   - Prices trending DOWN     [vv]\n";
    std::cout << "  volatile (vol, wild)    - Large price swings       [~~]\n";
    std::cout << "  sideways (calm, stable) - Range-bound, low drift   [==]\n";
    std::cout << "  choppy   (chop, mixed)  - Erratic movement         [//]\n";
    std::cout << "  neutral  (default)      - Balanced market          [--]\n";
    std::cout << "\nINTENSITY LEVELS:\n";
    std::cout << "  mild       (low, soft)  - Subtle effects           [.]  (0.4x)\n";
    std::cout << "  moderate   (med)        - Noticeable effects       [o]  (0.7x)\n";
    std::cout << "  normal     (default)    - Standard effects         [O]  (1.0x)\n";
    std::cout << "  aggressive (high, agg)  - Strong effects           [*]  (1.2x)\n";
    std::cout << "  extreme    (max, crazy) - DRAMATIC effects         [!]  (1.6x)\n";
    std::cout << "\nKEYBOARD CONTROLS (during runtime):\n";
    std::cout << "  1-6       - Switch sentiment (1=Bull 2=Bear 3=Vol 4=Sideways 5=Chop 6=Neutral)\n";
    std::cout << "  M/N/A/X   - Set intensity (M=Mild N=Normal A=Aggressive X=eXtreme)\n";
    std::cout << "  + / -     - Increase/decrease spread\n";
    std::cout << "  P         - Pause/Resume simulation\n";
    std::cout << "  F / S     - Faster/Slower (speed 0.25x - 4x)\n";
    std::cout << "  SPACE     - Cycle to next sentiment\n";
    std::cout << "  TAB       - Cycle to next intensity\n";
    std::cout << "  Q / ESC   - Quit\n";
    std::cout << "\nEXAMPLES:\n";
    std::cout << "  orderbook.exe -i                                    (Interactive setup)\n";
    std::cout << "  orderbook.exe -p 250 -s AAPL --sentiment bullish    (Apple at $250, bullish)\n";
    std::cout << "  orderbook.exe --spread 0.10 --intensity aggressive  (Wide spread, aggressive)\n";
    std::cout << "  orderbook.exe --headless --auto-start               (WebSocket mode, no UI)\n";
    std::cout << "================================================================\n\n";
}

// ============================================================================
// INTERACTIVE CONFIGURATION
// ============================================================================

void getConfigInteractive(SimulationConfig& config) {
    std::string input;
    
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "         INTERACTIVE SIMULATION SETUP\n";
    std::cout << "================================================================\n";
    std::cout << "Press ENTER to accept default values shown in [brackets]\n\n";
    
    // Stock Symbol
    std::cout << "Stock Symbol [" << config.stockSymbol << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        // Convert to uppercase and limit to 6 chars
        for (char& c : input) c = std::toupper(c);
        config.stockSymbol = input.substr(0, 6);
    }
    
    // Base Price
    std::cout << "Base Price ($100-$500) [" << std::fixed << std::setprecision(2) << config.basePrice << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            config.basePrice = std::stod(input);
        } catch (...) {}
    }
    
    // Spread
    std::cout << "Spread ($0.05-$0.25) [" << std::fixed << std::setprecision(2) << config.spread << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            config.spread = std::stod(input);
        } catch (...) {}
    }
    
    // Sentiment
    std::cout << "\nSentiments: 1=Bullish, 2=Bearish, 3=Volatile, 4=Calm, 5=Choppy, 6=Neutral\n";
    std::cout << "Sentiment [6 - Neutral]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        switch (input[0]) {
            case '1': config.sentiment = Sentiment::BULLISH; break;
            case '2': config.sentiment = Sentiment::BEARISH; break;
            case '3': config.sentiment = Sentiment::VOLATILE; break;
            case '4': config.sentiment = Sentiment::CALM; break;
            case '5': config.sentiment = Sentiment::CHOPPY; break;
            case '6': config.sentiment = Sentiment::NEUTRAL; break;
            default:
                config.sentiment = MarketSentimentController::parseSentiment(input);
        }
    }
    
    // Intensity
    std::cout << "\nIntensities: 1=Mild, 2=Moderate, 3=Normal, 4=Aggressive, 5=Extreme\n";
    std::cout << "Intensity [3 - Normal]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        switch (input[0]) {
            case '1': config.intensity = Intensity::MILD; break;
            case '2': config.intensity = Intensity::MODERATE; break;
            case '3': config.intensity = Intensity::NORMAL; break;
            case '4': config.intensity = Intensity::AGGRESSIVE; break;
            case '5': config.intensity = Intensity::EXTREME; break;
            default:
                config.intensity = MarketSentimentController::parseIntensity(input);
        }
    }
    
    // Speed
    std::cout << "\nSpeed (0.25x - 4x) [" << config.speedMultiplier << "]: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            config.speedMultiplier = std::stod(input);
        } catch (...) {}
    }
    
    // Validate all values
    config.validate();
    
    std::cout << "\n================================================================\n";
    std::cout << "  Configuration Summary:\n";
    std::cout << "  Symbol:     " << config.stockSymbol << "\n";
    std::cout << "  Base Price: $" << std::fixed << std::setprecision(2) << config.basePrice << "\n";
    std::cout << "  Spread:     $" << config.spread << "\n";
    std::cout << "  Sentiment:  " << MarketSentimentController::getSentimentName(config.sentiment) << "\n";
    std::cout << "  Intensity:  " << MarketSentimentController::getIntensityName(config.intensity) << "\n";
    std::cout << "  Speed:      " << config.speedMultiplier << "x\n";
    std::cout << "================================================================\n";
}

// ============================================================================
// PARSE COMMAND LINE ARGUMENTS
// ============================================================================

bool parseCommandLine(int argc, char* argv[], SimulationConfig& config, bool& interactive) {
    interactive = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help" || arg == "/?") {
            return false;  // Show help
        }
        else if (arg == "-i" || arg == "--interactive") {
            interactive = true;
        }
        else if ((arg == "-p" || arg == "--price") && i + 1 < argc) {
            try {
                config.basePrice = std::stod(argv[++i]);
            } catch (...) {}
        }
        else if ((arg == "-s" || arg == "--symbol") && i + 1 < argc) {
            config.stockSymbol = argv[++i];
            for (char& c : config.stockSymbol) c = std::toupper(c);
        }
        else if (arg == "--spread" && i + 1 < argc) {
            try {
                config.spread = std::stod(argv[++i]);
            } catch (...) {}
        }
        else if (arg == "--sentiment" && i + 1 < argc) {
            config.sentiment = MarketSentimentController::parseSentiment(argv[++i]);
        }
        else if (arg == "--intensity" && i + 1 < argc) {
            config.intensity = MarketSentimentController::parseIntensity(argv[++i]);
        }
        else if (arg == "--speed" && i + 1 < argc) {
            try {
                config.speedMultiplier = std::stod(argv[++i]);
            } catch (...) {}
        }
        else if (arg == "--auto-start" || arg == "-a") {
            config.autoStart = true;
        }
        else if (arg == "--wait-for-ws" || arg == "-w") {
            config.waitForWebSocket = true;
            config.autoStart = true;  // Implied
        }
        else if (arg == "--headless") {
            config.headless = true;
            config.autoStart = true;  // Implied
        }
        else if (arg == "--debug" || arg == "-d") {
            config.debug = true;
            g_debug = true;
        }
        // Legacy support: first positional arg is sentiment, second is intensity
        else if (i == 1 && arg[0] != '-') {
            config.sentiment = MarketSentimentController::parseSentiment(arg);
        }
        else if (i == 2 && arg[0] != '-') {
            config.intensity = MarketSentimentController::parseIntensity(arg);
        }
    }
    
    config.validate();
    return true;
}

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool interactive = false;
    if (!parseCommandLine(argc, argv, g_config, interactive)) {
        printUsage(argv[0]);
        return 0;
    }
    
    // Interactive setup if requested
    if (interactive) {
        getConfigInteractive(g_config);
    }
    
    // Apply configuration
    g_sentimentController.setMarketCondition(g_config.sentiment, g_config.intensity);
    g_sentimentController.setSpread(g_config.spread);
    g_speedMultiplier = g_config.speedMultiplier;
    g_lastLoggedSentiment = g_config.sentiment;
    g_lastLoggedIntensity = g_config.intensity;
    
    // Initialize price logging
    initPriceLog();
    
    if (!interactive) {
        printUsage(argv[0]);
    }
    
    std::cout << "\n================================================================\n";
    std::cout << "  STARTING SIMULATION\n";
    std::cout << "  Symbol:     " << g_config.stockSymbol << "\n";
    std::cout << "  Base Price: $" << std::fixed << std::setprecision(2) << g_config.basePrice << "\n";
    std::cout << "  Spread:     $" << g_config.spread << "\n";
    std::cout << "  Sentiment:  " << g_sentimentController.getMarketConditionString() << "\n";
    std::cout << "  Speed:      " << g_config.speedMultiplier << "x\n";
    std::cout << "================================================================\n";
    
    if (!g_config.autoStart) {
        std::cout << "Press any key to start...\n";
#ifdef _WIN32
        _getch();
#else
        std::cin.get();
#endif
    } else {
        std::cout << "Auto-starting simulation...\n";
    }
    
    // Set up signal handler for graceful shutdown
    std::signal(SIGINT, signalHandler);
    
    // Start WebSocket server FIRST if waiting for frontend
    #ifdef WEBSOCKET_ENABLED
    // Read PORT from environment (for cloud deployment) or use default 8080
    int wsPort = 8080;
    const char* portEnv = std::getenv("PORT");
    if (portEnv) {
        wsPort = std::atoi(portEnv);
        std::cout << "[Server] Using PORT from environment: " << wsPort << "\n";
    }
    WebSocketServer wsServer(wsPort);
    g_wsServer = &wsServer;
    
    // Set up command callback for WebSocket - now handles per-session state
    wsServer.setCommandCallback([&](uint32_t clientId, const std::string& type, const std::string& value) {
        // Get the session for this client
        SessionState* session = wsServer.getSession(clientId);
        if (!session) {
            std::cout << "[Session " << clientId << "] [WARN] No session found\n";
            return;
        }
        
        // Don't log ping commands (too noisy)
        if (type != "ping") {
            std::cout << "[Session " << clientId << "] [COMMAND] " << type << "=" << value << std::endl;
        }
        
        if (type == "sentiment") {
            Sentiment s = MarketSentimentController::parseSentiment(value);
            session->setSentiment(s);
            std::cout << "[Session " << clientId << "] [SET] Sentiment -> " << value << std::endl;
        } else if (type == "intensity") {
            Intensity i = MarketSentimentController::parseIntensity(value);
            session->setIntensity(i);
            std::cout << "[Session " << clientId << "] [SET] Intensity -> " << value << std::endl;
        } else if (type == "spread") {
            try {
                double spread = std::stod(value);
                session->setSpread(spread);
                std::cout << "[Session " << clientId << "] [SET] Spread -> $" << std::fixed << std::setprecision(2) << spread << std::endl;
            } catch (...) {}
        } else if (type == "speed") {
            try {
                double speed = std::stod(value);
                session->setSpeed(speed);
                std::cout << "[Session " << clientId << "] [SET] Speed -> " << speed << "x" << std::endl;
            } catch (...) {}
        } else if (type == "pause") {
            session->setPaused(value == "true" || value == "1");
            std::cout << "[Session " << clientId << "] [STATE] " << (session->isPaused() ? "PAUSED" : "RESUMED") << std::endl;
        } else if (type == "newsShock") {
            // News Shock toggle
            if (value == "true") {
                if (session->getNewsShockController().enable()) {
                    std::cout << "[Session " << clientId << "] [STATE] News Shock ENABLED (5s)\n";
                } else {
                    std::cout << "[Session " << clientId << "] [WARN] News Shock in cooldown\n";
                }
            } else {
                session->getNewsShockController().disable();
                std::cout << "[Session " << clientId << "] [STATE] News Shock DISABLED\n";
            }
        } else if (type == "reset") {
            session->reset();
            std::cout << "[Session " << clientId << "] [INFO] Simulation RESET\n";
            // Send reset confirmation to clear frontend state
            g_wsServer->sendToClient(clientId, R"({"type":"simulationReset"})");
            g_wsServer->sendToClient(clientId, R"({"type":"candleReset"})");
        } else if (type == "symbol") {
            std::string symbol = value;
            for (char& c : symbol) c = std::toupper(c);
            session->setSymbol(symbol);
            std::cout << "[Session " << clientId << "] [SET] Symbol -> " << symbol << std::endl;
        } else if (type == "price") {
            try {
                SessionConfig config = session->getConfig();
                config.basePrice = std::stod(value);
                config.validate();
                session->setConfig(config);
                session->reset(); // Reset with new base price
                std::cout << "[Session " << clientId << "] [SET] Base Price -> $" << std::fixed << std::setprecision(2) << config.basePrice << std::endl;
                // Send reset confirmation to clear frontend state
                g_wsServer->sendToClient(clientId, R"({"type":"simulationReset"})");
                g_wsServer->sendToClient(clientId, R"({"type":"candleReset"})");
            } catch (...) {}
        } else if (type == "getCandles") {
            // Handle getCandles request - send cached candles for this session's timeframe
            try {
                int timeframe = std::stoi(value);
                const auto& candles = session->getCandleManager().getCachedCandles(timeframe);
                const auto* current = session->getCandleManager().getCurrentCandle(timeframe);
                std::string response = JsonBuilder::candleHistoryToJson(timeframe, candles, current);
                g_wsServer->sendToClient(clientId, response);
                std::cout << "[Session " << clientId << "] [INFO] Sent " << candles.size() << " candles (" << timeframe << "s)\n";
            } catch (...) {
                std::cout << "[Session " << clientId << "] [ERROR] Invalid timeframe in getCandles\n";
            }
        } else if (type == "start") {
            session->setRunning(true);
            g_wsStartReceived = true;  // Still signal for any waiting
            std::cout << "[Session " << clientId << "] [INFO] Simulation STARTED\n";
            // Send confirmation to client
            g_wsServer->sendToClient(clientId, R"({"type":"started"})");
        } else if (type == "ping") {
            // Respond immediately with pong for latency measurement
            g_wsServer->sendToClient(clientId, R"({"type":"pong","timestamp":)" + value + "}");
        } else if (type == "stats") {
            // Print stats for this session
            std::cout << "[Session " << clientId << "] [STATS] " << g_wsServer->getSessionStatsString(clientId) << "\n";
        }
    });
    
    if (wsServer.start()) {
        std::cout << "[Server] [INFO] WebSocket server running on ws://0.0.0.0:" << wsPort << std::endl;
        std::cout.flush();
    } else {
        std::cout << "[Server] [ERROR] Failed to start WebSocket server\n";
        std::cout.flush();
    }
    
    // If waiting for WebSocket, pause until start signal
    if (g_config.waitForWebSocket) {
        std::cout << "\nWaiting for frontend connection...\n";
        std::cout << "Open http://localhost:5173 and click Start\n\n";
        
        while (!g_wsStartReceived && g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (!g_running) {
            wsServer.stop();
            return 0;
        }
        
        // Apply config from WebSocket
        g_sentimentController.setMarketCondition(g_config.sentiment, g_config.intensity);
        g_sentimentController.setSpread(g_config.spread);
        g_speedMultiplier = g_config.speedMultiplier;
        
        std::cout << "\n================================================================\n";
        std::cout << "  SIMULATION STARTED FROM FRONTEND\n";
        std::cout << "  Symbol:     " << g_config.stockSymbol << "\n";
        std::cout << "  Base Price: $" << std::fixed << std::setprecision(2) << g_config.basePrice << "\n";
        std::cout << "  Spread:     $" << g_config.spread << "\n";
        std::cout << "  Sentiment:  " << g_sentimentController.getMarketConditionString() << "\n";
        std::cout << "  Speed:      " << g_config.speedMultiplier << "x\n";
        std::cout << "================================================================\n\n";
    }
    #endif
    
    // Create core components (AFTER WebSocket config is applied)
    OrderBook orderBook;
    MatchingEngine engine(orderBook);
    OrderQueue orderQueue;
    Visualizer visualizer(orderBook, g_config.stockSymbol);
    visualizer.setSentimentController(&g_sentimentController);
    
    // Create the order generator with configured base price
    double basePrice = g_config.basePrice;
    SentimentOrderGenerator generator(g_sentimentController, basePrice);
    g_generator = &generator;
    
    // Counters
    std::atomic<OrderId> nextOrderId{1};
    std::atomic<size_t> processedCount{0};
    std::atomic<size_t> marketOrderCount{0};
    std::atomic<size_t> limitOrderCount{0};
    
    // Pre-populate the order book with limit orders to create initial liquidity
    // All prices are on $0.05 tick increments, centered around base price
    std::cout << "Building initial order book (tick size: $0.05)...\n";
    for (int i = 0; i < 20; ++i) {
        // Prices at $0.05 increments centered around base price
        double bidPrice = basePrice - 0.05 - (i * 0.05);  // Start just below base
        double askPrice = basePrice + 0.05 + (i * 0.05);  // Start just above base
        int qty = 100 + i * 20;
        
        orderBook.addOrder(Order(nextOrderId++, Side::BUY, OrderType::LIMIT, bidPrice, qty));
        orderBook.addOrder(Order(nextOrderId++, Side::SELL, OrderType::LIMIT, askPrice, qty));
    }
    
    // Log initial state
    logPrice(basePrice, "SESSION_START");
    
    // Initialize price tracking
    g_openPrice = basePrice;
    g_highPrice = basePrice;
    g_lowPrice = basePrice;
    g_currentPrice = basePrice;
    
    // Start threads
    std::thread generatorThread(orderGenerator, std::ref(orderQueue), 
                                std::ref(nextOrderId), std::ref(orderBook));
    std::thread processorThread(orderProcessor, std::ref(orderQueue), 
                                std::ref(engine), std::ref(visualizer),
                                std::ref(processedCount),
                                std::ref(marketOrderCount),
                                std::ref(limitOrderCount),
                                std::ref(orderBook));
    std::thread displayThread(displayUpdater, std::ref(visualizer), 
                              std::ref(processedCount), std::ref(engine),
                              std::ref(marketOrderCount),
                              std::ref(limitOrderCount),
                              std::ref(orderBook));
    std::thread keyboardThread(keyboardHandler);
    
    // Wait for threads to finish
    generatorThread.join();
    orderQueue.shutdown();
    processorThread.join();
    displayThread.join();
    keyboardThread.join();
    
    // Stop WebSocket server
    #ifdef WEBSOCKET_ENABLED
    // Print final stats before stopping
    std::cout << "\n[Server] [INFO] Shutting down...\n";
    wsServer.printStats();
    wsServer.stop();
    g_wsServer = nullptr;
    #endif
    
    // Log final price
    {
        std::lock_guard<std::mutex> lock(g_generatorMutex);
        if (g_generator) {
            logPrice(g_generator->getLastTradePrice(), "SESSION_END");
        }
    }
    
    // Clean up
    g_generator = nullptr;
    closePriceLog();
    
    // Final statistics
    std::cout << "\n========================================\n";
    std::cout << "   Shutdown Complete\n";
    std::cout << "========================================\n";
    std::cout << "Symbol: " << g_config.stockSymbol << "\n";
    std::cout << "Total Orders Processed: " << processedCount.load() << "\n";
    std::cout << "  - Market Orders: " << marketOrderCount.load() << " (" 
              << (processedCount.load() > 0 ? 100*marketOrderCount.load()/processedCount.load() : 0) << "%)\n";
    std::cout << "  - Limit Orders:  " << limitOrderCount.load() << " (" 
              << (processedCount.load() > 0 ? 100*limitOrderCount.load()/processedCount.load() : 0) << "%)\n";
    std::cout << "Total Trades Executed:  " << engine.getTradeCount() << "\n";
    std::cout << "Total Volume Traded:    " << engine.getTotalVolume() << "\n";
    std::cout << "========================================\n\n";
    
    return 0;
}
