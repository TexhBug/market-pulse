// ============================================================================
// SESSION STATE - Per-client simulation state management
// ============================================================================
// Each WebSocket connection gets its own isolated simulation state
// ============================================================================

#ifndef SESSION_STATE_H
#define SESSION_STATE_H

#include <string>
#include <atomic>
#include <chrono>
#include <memory>
#include <cstdlib>
#include "MarketSentiment.h"
#include "PriceEngine.h"
#include "CandleManager.h"
#include "NewsShock.h"
#include "OrderBook.h"

namespace orderbook {

// ============================================================================
// Trade Data (for visualization)
// ============================================================================

struct TradeData {
    int64_t id;
    double price;
    int quantity;
    std::string side;  // "BUY" or "SELL"
    int64_t timestamp;
    
    bool isValid() const { return id > 0; }
};

// ============================================================================
// Session Configuration
// ============================================================================

struct SessionConfig {
    std::string symbol = "DEMO";
    double basePrice = 100.0;
    double spread = 0.05;
    Sentiment sentiment = Sentiment::NEUTRAL;
    Intensity intensity = Intensity::NORMAL;
    double speed = 1.0;
    
    void validate() {
        basePrice = std::max(100.0, std::min(500.0, basePrice));
        spread = std::max(0.05, std::min(0.25, spread));
        speed = std::max(0.25, std::min(2.0, speed));
        // Round to tick size
        basePrice = std::round(basePrice / 0.05) * 0.05;
        spread = std::round(spread / 0.05) * 0.05;
    }
};

// ============================================================================
// Session State - Per-client simulation
// Note: This is for visualization only, no actual matching engine needed
// ============================================================================

class SessionState {
public:
    explicit SessionState(uint32_t sessionId, const SessionConfig& config = SessionConfig())
        : m_sessionId(sessionId)
        , m_config(config)
        , m_running(false)
        , m_paused(false)
    {
        m_config.validate();
        reset();
    }
    
    // Session ID
    uint32_t getId() const { return m_sessionId; }
    
    // Configuration
    const SessionConfig& getConfig() const { return m_config; }
    void setConfig(const SessionConfig& config) {
        m_config = config;
        m_config.validate();
    }
    
    // Control
    bool isRunning() const { return m_running; }
    void setRunning(bool running) { m_running = running; }
    
    bool isPaused() const { return m_paused; }
    void setPaused(bool paused) { m_paused = paused; }
    
    // Symbol
    const std::string& getSymbol() const { return m_config.symbol; }
    void setSymbol(const std::string& symbol) { m_config.symbol = symbol; }
    
    // Spread
    double getSpread() const { return m_config.spread; }
    void setSpread(double spread) {
        m_config.spread = std::max(0.05, std::min(0.25, spread));
    }
    
    // Speed
    double getSpeed() const { return m_config.speed; }
    void setSpeed(double speed) {
        m_config.speed = std::max(0.25, std::min(2.0, speed));
    }
    
    // Sentiment
    Sentiment getSentiment() const { return m_sentimentController.getSentiment(); }
    void setSentiment(Sentiment s) { m_sentimentController.setSentiment(s); }
    
    Intensity getIntensity() const { return m_sentimentController.getIntensity(); }
    void setIntensity(Intensity i) { m_sentimentController.setIntensity(i); }
    
    // Price tracking
    double getCurrentPrice() const { return m_currentPrice; }
    void setCurrentPrice(double price) {
        m_currentPrice = price;
        if (price > m_highPrice) m_highPrice = price;
        if (price < m_lowPrice) m_lowPrice = price;
    }
    
    double getOpenPrice() const { return m_openPrice; }
    double getHighPrice() const { return m_highPrice; }
    double getLowPrice() const { return m_lowPrice; }
    
    // Statistics
    size_t getTotalOrders() const { return m_totalOrders; }
    void addOrders(size_t count) { m_totalOrders += count; }
    
    size_t getTotalTrades() const { return m_totalTrades; }
    void addTrade() { m_totalTrades++; }
    
    // Generate a trade for visualization
    TradeData generateTrade(double currentPrice, int64_t timestamp) {
        m_tradeCounter++;
        m_totalTrades++;
        
        // Create globally unique trade ID: sessionId * 1000000 + counter
        int64_t uniqueId = static_cast<int64_t>(m_sessionId) * 1000000 + m_tradeCounter;
        
        // Buy probability based on sentiment
        double buyProb = getBuyProbability(MarketSentimentController::getSentimentNameSimple(getSentiment()));
        bool isBuy = (rand() / (double)RAND_MAX) < buyProb;
        
        // Small slippage for realism
        double slippage = (rand() / (double)RAND_MAX) * 0.02 + 0.01;
        double price = currentPrice + (isBuy ? slippage : -slippage);
        // Round to tick size (0.05)
        price = std::round(price * 20.0) / 20.0;
        
        // Quantity based on intensity
        double volMultiplier = getVolumeMultiplier(
            MarketSentimentController::getIntensityNameSimple(getIntensity()));
        int baseQty = 10 + (rand() % 100);
        int quantity = static_cast<int>(baseQty * volMultiplier);
        
        return TradeData{
            uniqueId,
            price,
            quantity,
            isBuy ? "BUY" : "SELL",
            timestamp
        };
    }
    
    size_t getTotalVolume() const { return m_totalVolume; }
    void addVolume(size_t volume) { m_totalVolume += volume; }
    
    int getMarketOrderPct() const {
        size_t total = m_marketOrders + m_limitOrders;
        return (total > 0) ? static_cast<int>(100.0 * m_marketOrders / total) : 0;
    }
    void addMarketOrder() { m_marketOrders++; }
    void addLimitOrder() { m_limitOrders++; }
    
    // Components access
    MarketSentimentController& getSentimentController() { return m_sentimentController; }
    PriceEngine& getPriceEngine() { return m_priceEngine; }
    CandleManager& getCandleManager() { return m_candleManager; }
    NewsShockController& getNewsShockController() { return m_newsShockController; }
    OrderBook& getOrderBook() { return m_orderBook; }
    
    const MarketSentimentController& getSentimentController() const { return m_sentimentController; }
    const PriceEngine& getPriceEngine() const { return m_priceEngine; }
    const CandleManager& getCandleManager() const { return m_candleManager; }
    const NewsShockController& getNewsShockController() const { return m_newsShockController; }
    const OrderBook& getOrderBook() const { return m_orderBook; }
    
    // Reset session to initial state
    void reset() {
        m_currentPrice = m_config.basePrice;
        m_openPrice = m_config.basePrice;
        m_highPrice = m_config.basePrice;
        m_lowPrice = m_config.basePrice;
        m_totalOrders = 0;
        m_totalTrades = 0;
        m_totalVolume = 0;
        m_marketOrders = 0;
        m_limitOrders = 0;
        m_paused = false;
        
        m_sentimentController.setMarketCondition(m_config.sentiment, m_config.intensity);
        m_sentimentController.setSpread(m_config.spread);
        m_priceEngine.reset();
        m_candleManager.reset();
        m_newsShockController.reset();
        // Note: OrderBook doesn't have clear(), and we regenerate it each tick anyway
    }
    
    // Last update timestamp
    int64_t getLastUpdateTime() const { return m_lastUpdateTime; }
    void setLastUpdateTime(int64_t time) { m_lastUpdateTime = time; }
    
private:
    uint32_t m_sessionId;
    SessionConfig m_config;
    
    // Control state
    bool m_running;
    bool m_paused;
    
    // Price tracking
    double m_currentPrice = 100.0;
    double m_openPrice = 100.0;
    double m_highPrice = 100.0;
    double m_lowPrice = 100.0;
    
    // Statistics
    size_t m_totalOrders = 0;
    size_t m_totalTrades = 0;
    size_t m_totalVolume = 0;
    size_t m_marketOrders = 0;
    size_t m_limitOrders = 0;
    int64_t m_tradeCounter = 0;
    
    // Timestamp tracking
    int64_t m_lastUpdateTime = 0;
    
    // Per-session components
    MarketSentimentController m_sentimentController;
    PriceEngine m_priceEngine;
    CandleManager m_candleManager;
    NewsShockController m_newsShockController;
    OrderBook m_orderBook;
};

} // namespace orderbook

#endif // SESSION_STATE_H
