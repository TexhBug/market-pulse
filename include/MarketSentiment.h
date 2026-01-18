#ifndef MARKET_SENTIMENT_H
#define MARKET_SENTIMENT_H

#include "Common.h"  // For Side enum
#include "Order.h"   // For Order class
#include "OrderBook.h" // For OrderBook class
#include <string>
#include <map>
#include <random>
#include <atomic>
#include <algorithm>
#include <memory>

// Note: Do NOT use "using namespace" in headers - it pollutes the namespace
// Instead, we'll qualify orderbook:: types explicitly where needed

// ============================================================================
// MARKET SENTIMENT TYPES
// ============================================================================
// These represent different market conditions that affect order generation
// ============================================================================

enum class Sentiment {
    BULLISH,    // 📈 Prices trending up, more buyers
    BEARISH,    // 📉 Prices trending down, more sellers  
    VOLATILE,   // 🎢 Large price swings, high activity
    CALM,       // 😌 Stable prices, low activity
    CHOPPY,     // 🔀 Erratic, frequent direction changes
    NEUTRAL     // ⚖️  Balanced market (default)
};

// ============================================================================
// INTENSITY LEVELS - How aggressive the sentiment should be
// ============================================================================

enum class Intensity {
    MILD,       // 🌱 Subtle effects (0.3x multiplier)
    MODERATE,   // 🌿 Noticeable effects (0.6x multiplier)
    NORMAL,     // 🌳 Standard effects (1.0x multiplier) - DEFAULT
    AGGRESSIVE, // 🔥 Strong effects (1.5x multiplier)
    EXTREME     // 💥 Very dramatic effects (2.5x multiplier)
};

// ============================================================================
// MARKET PARAMETERS
// ============================================================================
// Configuration that controls how orders are generated
// ============================================================================

struct MarketParameters {
    // Order direction bias (0.0 = all sells, 0.5 = balanced, 1.0 = all buys)
    double buyProbability = 0.5;
    
    // Price movement
    double priceDrift = 0.0;           // How much price tends to move per order (-1.0 to 1.0)
    double priceVolatility = 0.05;     // How much price varies (standard deviation as %)
    
    // Order sizes
    int minQuantity = 50;
    int maxQuantity = 200;
    double largOrderProbability = 0.1; // Chance of a large "whale" order
    int largeOrderMultiplier = 5;      // How much bigger large orders are
    
    // Order rate (faster for responsive visualization)
    int minDelayMs = 10;               // Minimum delay between orders
    int maxDelayMs = 50;               // Maximum delay between orders
    
    // Spread behavior
    double spreadTightness = 1.0;      // 0.5 = tight spread, 2.0 = wide spread
    
    // Cancel/modify behavior
    double cancelProbability = 0.05;   // Chance to cancel existing order
    double modifyProbability = 0.03;   // Chance to modify existing order
    
    // Market order probability (crosses the spread immediately)
    double marketOrderProbability = 0.1;
};

// ============================================================================
// MARKET SENTIMENT CONTROLLER
// ============================================================================
// Manages current sentiment and provides parameters for order generation
// ============================================================================

class MarketSentimentController {
public:
    // Tick size - all prices must be multiples of this
    static constexpr double TICK_SIZE = 0.05;
    
    // Spread limits (must be multiples of TICK_SIZE)
    static constexpr double MIN_SPREAD = 0.05;
    static constexpr double MAX_SPREAD = 0.25;
    static constexpr double SPREAD_STEP = 0.05;
    
    // Helper: Round any price to nearest tick
    static double roundToTick(double price) {
        return std::round(price / TICK_SIZE) * TICK_SIZE;
    }
    
    MarketSentimentController() 
        : currentSentiment_(Sentiment::NEUTRAL)
        , currentIntensity_(Intensity::NORMAL)
        , currentSpread_(MIN_SPREAD) {
        initializePresets();
    }
    
    // Get/Set current sentiment
    Sentiment getSentiment() const { return currentSentiment_.load(); }
    
    void setSentiment(Sentiment sentiment) {
        currentSentiment_.store(sentiment);
    }
    
    // Get/Set intensity
    Intensity getIntensity() const { return currentIntensity_.load(); }
    
    void setIntensity(Intensity intensity) {
        currentIntensity_.store(intensity);
    }
    
    // Combined setter
    void setMarketCondition(Sentiment sentiment, Intensity intensity) {
        currentSentiment_.store(sentiment);
        currentIntensity_.store(intensity);
    }
    
    // Cycle to next sentiment (for keyboard control)
    void nextSentiment() {
        int current = static_cast<int>(currentSentiment_.load());
        int next = (current + 1) % 6;  // 6 sentiment types
        currentSentiment_.store(static_cast<Sentiment>(next));
    }
    
    // Cycle to next intensity (for keyboard control)
    void nextIntensity() {
        int current = static_cast<int>(currentIntensity_.load());
        int next = (current + 1) % 5;  // 5 intensity levels
        currentIntensity_.store(static_cast<Intensity>(next));
    }
    
    // Spread controls
    double getSpread() const { return currentSpread_.load(); }
    
    void setSpread(double spread) {
        spread = std::max(MIN_SPREAD, std::min(MAX_SPREAD, spread));
        currentSpread_.store(spread);
    }
    
    void increaseSpread() {
        double current = currentSpread_.load();
        double newSpread = std::min(MAX_SPREAD, current + SPREAD_STEP);
        currentSpread_.store(newSpread);
    }
    
    void decreaseSpread() {
        double current = currentSpread_.load();
        double newSpread = std::max(MIN_SPREAD, current - SPREAD_STEP);
        currentSpread_.store(newSpread);
    }
    
    // Get intensity multiplier
    // Values balanced to leave room for news shock impact
    static double getIntensityMultiplier(Intensity i) {
        switch (i) {
            case Intensity::MILD:       return 0.4;   // Gentle movements
            case Intensity::MODERATE:   return 0.7;   // Moderate activity
            case Intensity::NORMAL:     return 1.0;   // Baseline
            case Intensity::AGGRESSIVE: return 1.2;   // Noticeable but not overwhelming
            case Intensity::EXTREME:    return 1.6;   // Fast but leaves room for shocks
            default: return 1.0;
        }
    }
    
    // Get parameters for current sentiment WITH intensity applied
    MarketParameters getParameters() const {
        MarketParameters base = presets_.at(currentSentiment_.load());
        double mult = getIntensityMultiplier(currentIntensity_.load());
        
        // Apply intensity multiplier to key parameters
        // Scale drift (the main price movement factor)
        base.priceDrift *= mult;
        
        // Scale volatility
        base.priceVolatility *= mult;
        
        // Scale bias away from 0.5 (neutral)
        double biasDelta = (base.buyProbability - 0.5) * mult;
        base.buyProbability = std::clamp(0.5 + biasDelta, 0.1, 0.9);
        
        // Scale order sizes
        base.minQuantity = static_cast<int>(base.minQuantity * (0.5 + mult * 0.5));
        base.maxQuantity = static_cast<int>(base.maxQuantity * mult);
        
        // Scale whale probability - cap to prevent huge orders
        base.largOrderProbability = std::min(base.largOrderProbability * mult, 0.15);
        base.largeOrderMultiplier = std::min(static_cast<int>(base.largeOrderMultiplier * mult), 5);
        
        // Scale speed (inverse - higher intensity = faster)
        base.minDelayMs = std::max(5, static_cast<int>(base.minDelayMs / mult));
        base.maxDelayMs = std::max(20, static_cast<int>(base.maxDelayMs / mult));
        
        // Scale market order probability - cap at 25% to maintain book depth
        base.marketOrderProbability = std::min(base.marketOrderProbability * mult, 0.25);
        
        return base;
    }
    
    // Get simple sentiment name for WebSocket (matches frontend types)
    static std::string getSentimentNameSimple(Sentiment s) {
        switch (s) {
            case Sentiment::BULLISH:  return "BULLISH";
            case Sentiment::BEARISH:  return "BEARISH";
            case Sentiment::VOLATILE: return "VOLATILE";
            case Sentiment::CALM:     return "SIDEWAYS";  // Frontend uses SIDEWAYS for CALM
            case Sentiment::CHOPPY:   return "CHOPPY";
            case Sentiment::NEUTRAL:  return "NEUTRAL";
            default: return "NEUTRAL";
        }
    }
    
    // Get simple intensity name for WebSocket (matches frontend types)
    static std::string getIntensityNameSimple(Intensity i) {
        switch (i) {
            case Intensity::MILD:       return "MILD";
            case Intensity::MODERATE:   return "MODERATE";
            case Intensity::NORMAL:     return "NORMAL";
            case Intensity::AGGRESSIVE: return "AGGRESSIVE";
            case Intensity::EXTREME:    return "EXTREME";
            default: return "NORMAL";
        }
    }
    
    // Get sentiment name for display (ASCII-safe for Windows terminal)
    static std::string getSentimentName(Sentiment s) {
        switch (s) {
            case Sentiment::BULLISH:  return "BULLISH  [^^]";
            case Sentiment::BEARISH:  return "BEARISH  [vv]";
            case Sentiment::VOLATILE: return "VOLATILE [~~]";
            case Sentiment::CALM:     return "CALM     [==]";
            case Sentiment::CHOPPY:   return "CHOPPY   [//]";
            case Sentiment::NEUTRAL:  return "NEUTRAL  [--]";
            default: return "UNKNOWN";
        }
    }
    
    // Get intensity name for display (ASCII-safe)
    static std::string getIntensityName(Intensity i) {
        switch (i) {
            case Intensity::MILD:       return "MILD [.]";
            case Intensity::MODERATE:   return "MODERATE [o]";
            case Intensity::NORMAL:     return "NORMAL [O]";
            case Intensity::AGGRESSIVE: return "AGGRESSIVE [*]";
            case Intensity::EXTREME:    return "EXTREME [!]";
            default: return "NORMAL";
        }
    }
    
    std::string getCurrentSentimentName() const {
        return getSentimentName(currentSentiment_.load());
    }
    
    std::string getCurrentIntensityName() const {
        return getIntensityName(currentIntensity_.load());
    }
    
    // Get full market condition string
    std::string getMarketConditionString() const {
        return getCurrentSentimentName() + " | " + getCurrentIntensityName();
    }
    
    // Get sentiment color code for display
    static std::string getSentimentColor(Sentiment s) {
        switch (s) {
            case Sentiment::BULLISH:  return "\033[92m";  // Bright green
            case Sentiment::BEARISH:  return "\033[91m";  // Bright red
            case Sentiment::VOLATILE: return "\033[93m";  // Bright yellow
            case Sentiment::CALM:     return "\033[96m";  // Bright cyan
            case Sentiment::CHOPPY:   return "\033[95m";  // Bright magenta
            case Sentiment::NEUTRAL:  return "\033[97m";  // Bright white
            default: return "\033[0m";
        }
    }
    
    // Parse sentiment from string (for command line args)
    static Sentiment parseSentiment(const std::string& str) {
        std::string lower = str;
        for (auto& c : lower) c = std::tolower(c);
        
        if (lower == "bullish" || lower == "bull" || lower == "up") 
            return Sentiment::BULLISH;
        if (lower == "bearish" || lower == "bear" || lower == "down") 
            return Sentiment::BEARISH;
        if (lower == "volatile" || lower == "vol" || lower == "wild") 
            return Sentiment::VOLATILE;
        if (lower == "calm" || lower == "stable" || lower == "quiet" || lower == "sideways") 
            return Sentiment::CALM;
        if (lower == "choppy" || lower == "chop" || lower == "mixed") 
            return Sentiment::CHOPPY;
        
        return Sentiment::NEUTRAL;
    }
    
    // Parse intensity from string (for command line args)
    static Intensity parseIntensity(const std::string& str) {
        std::string lower = str;
        for (auto& c : lower) c = std::tolower(c);
        
        if (lower == "mild" || lower == "low" || lower == "soft" || lower == "gentle")
            return Intensity::MILD;
        if (lower == "moderate" || lower == "med" || lower == "medium")
            return Intensity::MODERATE;
        if (lower == "normal" || lower == "default" || lower == "standard")
            return Intensity::NORMAL;
        if (lower == "aggressive" || lower == "high" || lower == "strong" || lower == "agg")
            return Intensity::AGGRESSIVE;
        if (lower == "extreme" || lower == "max" || lower == "insane" || lower == "crazy")
            return Intensity::EXTREME;
        
        return Intensity::NORMAL;
    }

private:
    std::atomic<Sentiment> currentSentiment_;
    std::atomic<Intensity> currentIntensity_;
    std::atomic<double> currentSpread_;
    std::map<Sentiment, MarketParameters> presets_;
    
    void initializePresets() {
        // ====================================================================
        // BULLISH - Strong buying pressure, prices trending up
        // ====================================================================
        MarketParameters bullish;
        bullish.buyProbability = 0.70;        // 70% buy orders
        bullish.priceDrift = 0.005;           // 0.5% drift per order - VISIBLE UPTREND
        bullish.priceVolatility = 0.02;       // Moderate volatility
        bullish.minQuantity = 80;             // Larger buy orders
        bullish.maxQuantity = 300;
        bullish.largOrderProbability = 0.15;  // More whale buyers
        bullish.minDelayMs = 30;              // Fast order flow
        bullish.maxDelayMs = 150;
        bullish.spreadTightness = 0.8;        // Tighter spreads (confidence)
        bullish.marketOrderProbability = 0.12; // ~12% market orders (realistic)
        presets_[Sentiment::BULLISH] = bullish;
        
        // ====================================================================
        // BEARISH - Strong selling pressure, prices trending down
        // ====================================================================
        MarketParameters bearish;
        bearish.buyProbability = 0.30;        // 30% buy orders (70% sells)
        bearish.priceDrift = -0.005;          // -0.5% drift per order - VISIBLE DOWNTREND
        bearish.priceVolatility = 0.025;      // Higher volatility (fear)
        bearish.minQuantity = 100;            // Panic selling = larger orders
        bearish.maxQuantity = 400;
        bearish.largOrderProbability = 0.20;  // More whale sellers dumping
        bearish.minDelayMs = 20;              // Very fast (panic)
        bearish.maxDelayMs = 100;
        bearish.spreadTightness = 1.5;        // Wider spreads (uncertainty)
        bearish.cancelProbability = 0.10;     // More cancellations
        bearish.marketOrderProbability = 0.15; // ~15% market orders (panic selling)
        presets_[Sentiment::BEARISH] = bearish;
        
        // ====================================================================
        // VOLATILE - Wild swings both directions
        // ====================================================================
        MarketParameters volatile_;
        volatile_.buyProbability = 0.50;      // Balanced but chaotic
        volatile_.priceDrift = 0.0;           // No trend, just swings
        volatile_.priceVolatility = 0.05;     // HIGH volatility for swings
        volatile_.minQuantity = 50;
        volatile_.maxQuantity = 500;          // Huge range
        volatile_.largOrderProbability = 0.25; // Many whales active
        volatile_.largeOrderMultiplier = 8;   // Even bigger whales
        volatile_.minDelayMs = 10;            // Very fast
        volatile_.maxDelayMs = 50;            // Even faster!
        volatile_.spreadTightness = 2.0;      // Wide spreads
        volatile_.cancelProbability = 0.15;   // Lots of cancels
        volatile_.modifyProbability = 0.10;   // Lots of modifications
        volatile_.marketOrderProbability = 0.18; // ~18% market orders
        presets_[Sentiment::VOLATILE] = volatile_;
        
        // ====================================================================
        // CALM - Stable, predictable, low activity
        // ====================================================================
        MarketParameters calm;
        calm.buyProbability = 0.50;           // Perfectly balanced
        calm.priceDrift = 0.0;                // No drift
        calm.priceVolatility = 0.005;         // Very low volatility
        calm.minQuantity = 20;                // Small orders
        calm.maxQuantity = 100;
        calm.largOrderProbability = 0.02;     // Rare whales
        calm.minDelayMs = 100;                // Slow order flow
        calm.maxDelayMs = 250;
        calm.spreadTightness = 0.5;           // Very tight spreads
        calm.cancelProbability = 0.02;        // Few cancels
        calm.marketOrderProbability = 0.05;   // Mostly limit orders
        presets_[Sentiment::CALM] = calm;
        
        // ====================================================================
        // CHOPPY - Erratic, no clear direction but with sudden moves
        // ====================================================================
        MarketParameters choppy;
        choppy.buyProbability = 0.50;         // Balanced
        choppy.priceDrift = 0.0;              // No sustained drift
        choppy.priceVolatility = 0.03;        // Medium volatility
        choppy.minQuantity = 30;
        choppy.maxQuantity = 250;
        choppy.largOrderProbability = 0.15;   // More whales = sudden moves
        choppy.largeOrderMultiplier = 6;
        choppy.minDelayMs = 40;
        choppy.maxDelayMs = 150;
        choppy.spreadTightness = 1.3;         // Moderate spreads
        choppy.cancelProbability = 0.12;      // High cancel rate
        choppy.modifyProbability = 0.08;      // High modify rate
        choppy.marketOrderProbability = 0.12; // ~12% market orders
        presets_[Sentiment::CHOPPY] = choppy;
        
        // ====================================================================
        // NEUTRAL - Default balanced market
        // ====================================================================
        MarketParameters neutral;
        // All defaults from struct definition
        presets_[Sentiment::NEUTRAL] = neutral;
    }
};

// ============================================================================
// SENTIMENT-AWARE ORDER GENERATOR
// ============================================================================
// REALISTIC ORDER GENERATOR - How real markets work:
// ============================================================================
// 1. LIMIT ORDERS: Placed relative to CURRENT best bid/ask in the book
//    - Buy limits go AT or BELOW best bid (join the queue or go deeper)
//    - Sell limits go AT or ABOVE best ask (join the queue or go deeper)
//
// 2. MARKET ORDERS: Execute immediately against best available
//    - Market BUY consumes best ASK (and moves up if exhausted)
//    - Market SELL consumes best BID (and moves down if exhausted)
//
// 3. PRICE MOVEMENT: Driven by order IMBALANCE
//    - Bullish = more/bigger market BUYS = consumes asks = price rises
//    - Bearish = more/bigger market SELLS = consumes bids = price falls
// ============================================================================

class SentimentOrderGenerator {
public:
    SentimentOrderGenerator(MarketSentimentController& controller, double basePrice = 100.0)
        : controller_(controller)
        , basePrice_(MarketSentimentController::roundToTick(basePrice))
        , lastTradePrice_(MarketSentimentController::roundToTick(basePrice))
        , bestBid_(MarketSentimentController::roundToTick(basePrice) - MarketSentimentController::TICK_SIZE)
        , bestAsk_(MarketSentimentController::roundToTick(basePrice) + MarketSentimentController::TICK_SIZE)
        , gen_(std::random_device{}())
    {}
    
    struct GeneratedOrder {
        orderbook::Side side;
        double price;
        int quantity;
        bool isMarketOrder;  // true = execute at market, false = limit order
    };
    
    // ========================================================================
    // GENERATE LIMIT ORDER (30% of orders)
    // These ADD liquidity - placed relative to current market
    // ========================================================================
    GeneratedOrder generateLimitOrder() {
        MarketParameters params = controller_.getParameters();
        GeneratedOrder order;
        order.isMarketOrder = false;
        
        std::uniform_real_distribution<> prob(0.0, 1.0);
        
        // Side based on sentiment (bullish = more bids, bearish = more asks)
        // BUT: Always ensure minimum liquidity on both sides (at least 25% each side)
        double buyProb = params.buyProbability;
        buyProb = std::clamp(buyProb, 0.25, 0.75);  // Ensure 25-75% range for limit orders
        
        order.side = (prob(gen_) < buyProb) ? orderbook::Side::BUY : orderbook::Side::SELL;
        
        // Use spread from controller
        double spreadValue = controller_.getSpread();
        
        // Reference price is the mid-point between best bid and ask
        // If one side is missing, use the last trade price
        double midPrice = lastTradePrice_;
        if (bestBid_ > 0 && bestAsk_ > bestBid_) {
            midPrice = (bestBid_ + bestAsk_) / 2.0;
        }
        
        // How many ticks away from the spread? (0 = at best, 1-5 = deeper in book)
        std::uniform_int_distribution<> ticksAway(0, 5);
        double offset = ticksAway(gen_) * MarketSentimentController::TICK_SIZE;
        
        if (order.side == orderbook::Side::BUY) {
            // BID: at or below the mid - half spread
            order.price = midPrice - spreadValue / 2.0 - offset;
        } else {
            // ASK: at or above the mid + half spread
            order.price = midPrice + spreadValue / 2.0 + offset;
        }
        
        // Round and validate
        order.price = MarketSentimentController::roundToTick(order.price);
        if (order.price <= 0) order.price = MarketSentimentController::TICK_SIZE;
        
        // Limit order sizes (smaller, providing liquidity)
        std::uniform_int_distribution<> qtyDist(params.minQuantity / 2, params.maxQuantity / 2);
        order.quantity = qtyDist(gen_);
        
        return order;
    }
    
    // ========================================================================
    // GENERATE MARKET ORDER (70% of orders)  
    // These CONSUME liquidity and MOVE PRICE when levels are exhausted!
    // ========================================================================
    GeneratedOrder generateMarketOrder() {
        MarketParameters params = controller_.getParameters();
        GeneratedOrder order;
        order.isMarketOrder = true;
        
        std::uniform_real_distribution<> prob(0.0, 1.0);
        
        // Side based on sentiment - THIS IS WHERE PRICE MOVEMENT COMES FROM!
        // Bullish = more market BUYS = consume asks = price rises
        // Bearish = more market SELLS = consume bids = price falls
        double buyBias = params.buyProbability;
        
        // Apply sentiment drift as additional bias
        if (params.priceDrift > 0) {
            buyBias += 0.15;  // Bullish: more buyers
        } else if (params.priceDrift < 0) {
            buyBias -= 0.15;  // Bearish: more sellers
        }
        buyBias = std::clamp(buyBias, 0.15, 0.85);
        
        order.side = (prob(gen_) < buyBias) ? orderbook::Side::BUY : orderbook::Side::SELL;
        
        // Market orders: price set to GUARANTEE crossing the spread
        // Use last trade price as reference if book side is empty
        if (order.side == orderbook::Side::BUY) {
            // Price high enough to hit any ask
            double targetPrice = (bestAsk_ > 0) ? bestAsk_ : lastTradePrice_;
            order.price = targetPrice + 10.0;
        } else {
            // Price low enough to hit any bid
            double targetPrice = (bestBid_ > 0) ? bestBid_ : lastTradePrice_;
            order.price = std::max(MarketSentimentController::TICK_SIZE, targetPrice - 10.0);
        }
        order.price = MarketSentimentController::roundToTick(order.price);
        
        // Market order SIZE is key to price movement!
        // Larger orders = consume more levels = bigger price moves
        std::uniform_int_distribution<> qtyDist(params.minQuantity, params.maxQuantity);
        order.quantity = qtyDist(gen_);
        
        // Whale orders (large) have bigger market impact
        if (prob(gen_) < params.largOrderProbability) {
            order.quantity *= params.largeOrderMultiplier;
        }
        
        return order;
    }
    
    // ========================================================================
    // MAIN GENERATOR - 80% limit, 20% market orders (realistic ratio)
    // ========================================================================
    GeneratedOrder generateOrder() {
        std::uniform_real_distribution<> prob(0.0, 1.0);
        MarketParameters params = controller_.getParameters();
        
        // Real markets: ~80% limit orders (add liquidity), ~20% market orders (take liquidity)
        // Market order probability can increase with volatility/intensity
        double marketProb = params.marketOrderProbability;  // Base: 0.1-0.3 depending on sentiment
        
        if (prob(gen_) < marketProb) {
            return generateMarketOrder();
        } else {
            return generateLimitOrder();
        }
    }
    
    // ========================================================================
    // UPDATE FROM TRADE - Called when a trade executes
    // Just tracks the last trade price for display purposes
    // ========================================================================
    void onTradeExecuted(double tradePrice, orderbook::Side /*aggressorSide*/) {
        lastTradePrice_ = MarketSentimentController::roundToTick(tradePrice);
    }
    
    // ========================================================================
    // UPDATE FROM ORDER BOOK - CRITICAL! This is how we know current prices
    // Must be called frequently to keep generator in sync with actual book
    // ========================================================================
    void updateFromOrderBook(double bid, double ask) {
        if (bid > 0) bestBid_ = MarketSentimentController::roundToTick(bid);
        if (ask > 0) bestAsk_ = MarketSentimentController::roundToTick(ask);
        
        // Sanity check: ask must be > bid
        if (bestAsk_ <= bestBid_) {
            bestAsk_ = bestBid_ + MarketSentimentController::TICK_SIZE;
        }
    }
    
    // Get delay until next order (in milliseconds)
    int getNextDelay() {
        MarketParameters params = controller_.getParameters();
        std::uniform_int_distribution<> delayDist(params.minDelayMs, params.maxDelayMs);
        return delayDist(gen_);
    }
    
    // Getters
    double getLastTradePrice() const { return lastTradePrice_; }
    double getBestBid() const { return bestBid_; }
    double getBestAsk() const { return bestAsk_; }
    
    void setBasePrice(double price) { 
        price = MarketSentimentController::roundToTick(price);
        basePrice_ = price; 
        lastTradePrice_ = price;
        bestBid_ = price - MarketSentimentController::TICK_SIZE;
        bestAsk_ = price + MarketSentimentController::TICK_SIZE;
    }
    
    // ========================================================================
    // REGENERATE ORDER BOOK - For visualization (like Node.js generateOrderBook)
    // Creates synthetic bid/ask levels based on current price and spread
    // ========================================================================
    void regenerateOrderBook(orderbook::OrderBook& book, double currentPrice, double spread) {
        // Clear all existing orders first
        book.clear();
        
        // Ensure minimum spread is at least one tick
        double effectiveSpread = std::max(spread, MarketSentimentController::TICK_SIZE);
        double halfSpread = effectiveSpread / 2.0;
        
        double bestBidPrice = MarketSentimentController::roundToTick(currentPrice - halfSpread);
        double bestAskPrice = MarketSentimentController::roundToTick(currentPrice + halfSpread);
        
        // CRITICAL: Ensure bid and ask are never the same price (spread > 0)
        // This can happen when halfSpread < TICK_SIZE/2 due to rounding
        if (bestBidPrice >= bestAskPrice) {
            // Center around current price with minimum 1 tick spread
            double midPrice = MarketSentimentController::roundToTick(currentPrice);
            bestBidPrice = midPrice - MarketSentimentController::TICK_SIZE;
            bestAskPrice = midPrice + MarketSentimentController::TICK_SIZE;
        }
        
        // Generate 15 levels on each side
        const int levels = 15;
        std::uniform_int_distribution<> qtyDist(50, 500);
        static orderbook::OrderId orderId = 1000000; // Start with high IDs to avoid conflicts
        
        // Get sentiment for depth bias
        Sentiment s = controller_.getSentiment();
        
        // Bids (descending from best bid)
        for (int i = 0; i < levels; i++) {
            double price = bestBidPrice - (i * MarketSentimentController::TICK_SIZE);
            if (price > 0) {
                // Quantity tapers off further from touch
                int baseQty = qtyDist(gen_);
                int qty = baseQty * (levels - i) / levels;
                qty = std::max(10, qty);
                
                // Sentiment affects depth - bullish = more bid depth
                if (s == Sentiment::BULLISH) {
                    qty = static_cast<int>(qty * 1.3);
                } else if (s == Sentiment::BEARISH) {
                    qty = static_cast<int>(qty * 0.7);
                }
                
                orderbook::Order order(orderId++, orderbook::Side::BUY, 
                    orderbook::OrderType::LIMIT, price, qty);
                book.addOrder(order);
            }
        }
        
        // Asks (ascending from best ask)
        for (int i = 0; i < levels; i++) {
            double price = bestAskPrice + (i * MarketSentimentController::TICK_SIZE);
            int baseQty = qtyDist(gen_);
            int qty = baseQty * (levels - i) / levels;
            qty = std::max(10, qty);
            
            // Sentiment affects depth - bearish = more ask depth
            if (s == Sentiment::BEARISH) {
                qty = static_cast<int>(qty * 1.3);
            } else if (s == Sentiment::BULLISH) {
                qty = static_cast<int>(qty * 0.7);
            }
            
            orderbook::Order order(orderId++, orderbook::Side::SELL,
                orderbook::OrderType::LIMIT, price, qty);
            book.addOrder(order);
        }
        
        // Update our tracking
        bestBid_ = bestBidPrice;
        bestAsk_ = bestAskPrice;
        lastTradePrice_ = currentPrice;
    }

private:
    MarketSentimentController& controller_;
    double basePrice_;
    double lastTradePrice_;
    double bestBid_;   // Current best bid from the ACTUAL order book
    double bestAsk_;   // Current best ask from the ACTUAL order book
    std::mt19937 gen_;
};

#endif // MARKET_SENTIMENT_H
