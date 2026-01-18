// ============================================================================
// PRICEENGINE.H - Price Simulation Engine (ported from Node.js)
// ============================================================================
// Handles all price movement logic including trends, pullbacks, and shocks
// ============================================================================

#ifndef PRICE_ENGINE_H
#define PRICE_ENGINE_H

#include <string>
#include <cmath>
#include <random>

namespace orderbook {

// ============================================================================
// Configuration Constants (matching Node.js config.js)
// ============================================================================

struct PriceEngineConfig {
    // News Shock configuration
    static constexpr int NEWS_SHOCK_ACTIVE_DURATION = 5000;      // 5 seconds
    static constexpr int NEWS_SHOCK_COOLDOWN_DURATION = 20000;   // 20 seconds
    static constexpr double NEWS_SHOCK_TRIGGER_CHANCE = 0.03;    // 3% per tick
    static constexpr int NEWS_SHOCK_MIN_TICKS_BETWEEN = 20;      // ~2 seconds
    static constexpr double NEWS_SHOCK_MIN_PERCENT = 0.01;       // 1%
    static constexpr double NEWS_SHOCK_MAX_PERCENT = 0.03;       // 3%
    
    // Pullback configuration
    static constexpr int PULLBACK_MIN_THRESHOLD = 8;
    static constexpr int PULLBACK_MAX_THRESHOLD = 15;
    static constexpr int PULLBACK_MIN_DURATION = 2;
    static constexpr int PULLBACK_MAX_DURATION = 5;
    static constexpr double PULLBACK_MIN_FACTOR = 0.7;
    static constexpr double PULLBACK_MAX_FACTOR = 0.9;
};

// ============================================================================
// Sentiment Parameters - Fine-tuned for realistic market behavior
// ============================================================================

struct SentimentParams {
    double upProbability;      // Base probability of price going up (0.0-1.0)
    double baseVolatility;     // Base price change per tick (as decimal, e.g., 0.001 = 0.1%)
    double trendStrength;      // How strongly the trend persists (0.0-1.0)
    double reversalChance;     // Chance of sudden direction change (0.0-1.0)
    int maxConsecutive;        // Max consecutive moves before forced reversal
    bool meanReversion;        // Whether price tends to revert to a mean
};

inline SentimentParams getSentimentParams(const std::string& sentiment) {
    // BULLISH: Strong upward bias, moderate volatility, persistent trends
    // Price should steadily climb with occasional small pullbacks
    if (sentiment == "BULLISH") return { 
        0.62,    // 62% chance of up move (was 68%)
        0.0004,  // 0.04% base volatility (was 0.06%)
        0.80,    // Strong trend persistence
        0.08,    // 8% chance of sudden reversal (was 5%)
        10,      // Allow up to 10 consecutive moves (was 12)
        false    // No mean reversion
    };
    
    // BEARISH: Strong downward bias, moderate volatility, persistent trends
    // Price should steadily decline with occasional small bounces
    if (sentiment == "BEARISH") return { 
        0.38,    // 38% chance of up move (62% down, was 68%)
        0.0004,  // 0.04% base volatility (was 0.06%)
        0.80,    // Strong trend persistence
        0.08,    // 8% chance of sudden reversal (was 5%)
        10,      // Allow up to 10 consecutive moves (was 12)
        false    // No mean reversion
    };
    
    // VOLATILE: No directional bias, HIGH volatility, big swings both ways
    // Large price movements, can go either direction dramatically
    if (sentiment == "VOLATILE") return { 
        0.50,    // 50% up/down (no bias)
        0.0012,  // 0.12% base volatility (was 0.20%)
        0.65,    // Moderate trend persistence
        0.18,    // 18% chance of sudden reversal
        6,       // Shorter consecutive limit (was 8)
        false    // No mean reversion
    };
    
    // SIDEWAYS: No bias, VERY LOW volatility, mean-reverting
    // Price stays in a tight range, always pulls back toward center
    if (sentiment == "SIDEWAYS") return { 
        0.50,    // 50% up/down
        0.0002,  // 0.02% base volatility (VERY LOW)
        0.30,    // Weak trend persistence
        0.10,    // 10% reversal chance
        5,       // Short consecutive limit
        true     // MEAN REVERSION enabled
    };
    
    // CHOPPY: Frequent direction changes, medium volatility, erratic
    // Unpredictable, rapid reversals, no clear trend
    if (sentiment == "CHOPPY") return { 
        0.50,    // 50% base (but modified per-tick)
        0.0010,  // 0.10% base volatility
        0.20,    // Very weak trend persistence (lots of reversals)
        0.35,    // 35% chance of sudden reversal (HIGH)
        3,       // Only 3 consecutive moves allowed (VERY SHORT)
        false    // No mean reversion
    };
    
    // NEUTRAL: Balanced, low volatility, moderate behavior
    return { 
        0.50,    // 50% up/down
        0.0004,  // 0.04% base volatility
        0.50,    // Moderate trend persistence
        0.10,    // 10% reversal chance
        8,       // Standard consecutive limit
        false    // No mean reversion
    };
}

// Intensity affects how strong the price movements are
inline double getIntensityMultiplier(const std::string& intensity) {
    if (intensity == "MILD") return 0.4;       // Subdued moves
    if (intensity == "MODERATE") return 0.7;   // Calmer than normal
    if (intensity == "AGGRESSIVE") return 1.0; // Slightly amplified (was 1.2)
    if (intensity == "EXTREME") return 1.25;   // Noticeably larger (was 1.6)
    return 0.85; // NORMAL - baseline slightly reduced
}

// Volume multiplier for trade generation
inline double getVolumeMultiplier(const std::string& intensity) {
    if (intensity == "MILD") return 0.5;
    if (intensity == "MODERATE") return 0.8;
    if (intensity == "AGGRESSIVE") return 1.2;
    if (intensity == "EXTREME") return 1.5;
    return 1.0; // NORMAL
}

// Buy probability for trade generation (affects trade ticker)
inline double getBuyProbability(const std::string& sentiment) {
    if (sentiment == "BULLISH") return 0.72;   // More buyers
    if (sentiment == "BEARISH") return 0.28;   // More sellers
    if (sentiment == "VOLATILE") return 0.50;  // Balanced chaos
    if (sentiment == "SIDEWAYS") return 0.50;  // Balanced
    if (sentiment == "CHOPPY") return 0.40 + (rand() / (double)RAND_MAX) * 0.20; // Random 40-60%
    return 0.50; // NEUTRAL
}

struct DepthMultipliers {
    double bidMultiplier;
    double askMultiplier;
};

inline DepthMultipliers getDepthMultipliers(const std::string& sentiment) {
    if (sentiment == "BULLISH") return { 1.5, 0.7 };
    if (sentiment == "BEARISH") return { 0.7, 1.5 };
    if (sentiment == "VOLATILE") return { 0.6, 0.6 };
    if (sentiment == "SIDEWAYS") return { 1.3, 1.3 };
    if (sentiment == "CHOPPY") {
        double r1 = 0.8 + (rand() / (double)RAND_MAX) * 0.6;
        double r2 = 0.8 + (rand() / (double)RAND_MAX) * 0.6;
        return { r1, r2 };
    }
    return { 1.0, 1.0 }; // NEUTRAL
}

// ============================================================================
// Price Engine Result
// ============================================================================

struct PriceResult {
    double newPrice;
    bool shockApplied;
    std::string shockType; // "bullish", "bearish", or ""
    double shockPercent;   // 0.0 if no shock
};

// ============================================================================
// Price Engine Class
// ============================================================================

class PriceEngine {
public:
    PriceEngine() : m_rng(std::random_device{}()) {
        reset();
    }
    
    /**
     * Calculate next price based on sentiment, intensity, and shock state
     */
    PriceResult calculateNextPrice(
        double currentPrice,
        const std::string& sentiment,
        const std::string& intensity,
        bool newsShockEnabled
    ) {
        double intensityMult = getIntensityMultiplier(intensity);
        auto params = getSentimentParams(sentiment);
        
        PriceResult result;
        result.newPrice = currentPrice;
        result.shockApplied = false;
        result.shockType = "";
        result.shockPercent = 0.0;
        
        m_ticksSinceLastShock++;
        
        // === NEWS SHOCK LOGIC ===
        if (newsShockEnabled) {
            if (m_ticksSinceLastShock >= PriceEngineConfig::NEWS_SHOCK_MIN_TICKS_BETWEEN &&
                randomDouble() < PriceEngineConfig::NEWS_SHOCK_TRIGGER_CHANCE) {
                
                // SHOCK EVENT!
                result.shockApplied = true;
                
                // Direction biased by sentiment
                double shockUpChance = params.upProbability; // Use sentiment bias
                int shockDirection = randomDouble() < shockUpChance ? 1 : -1;
                
                // Shock percentage: 1-3%, amplified by intensity
                result.shockPercent = (PriceEngineConfig::NEWS_SHOCK_MIN_PERCENT +
                    randomDouble() * (PriceEngineConfig::NEWS_SHOCK_MAX_PERCENT - 
                                      PriceEngineConfig::NEWS_SHOCK_MIN_PERCENT)) * intensityMult;
                
                // Apply shock directly
                result.newPrice = currentPrice * (1.0 + shockDirection * result.shockPercent);
                // Round to nearest 0.05 (tick size)
                result.newPrice = std::round(result.newPrice * 20.0) / 20.0;
                
                // Record shock
                result.shockType = shockDirection > 0 ? "bullish" : "bearish";
                m_lastShockType = result.shockType;
                m_ticksSinceLastShock = 0;
                
                // Reset trend state after shock
                m_consecutiveMoves = 0;
                m_pullbackRemaining = 0;
                
                return result;
            }
        }
        
        // Track anchor price for mean reversion (SIDEWAYS)
        if (m_anchorPrice <= 0.0) {
            m_anchorPrice = currentPrice;
        }
        
        // Normal price movement using full sentiment params
        auto moveResult = calculateNormalMove(currentPrice, params, intensityMult, sentiment);
        result.newPrice = currentPrice * (1.0 + moveResult.change);
        // Round to nearest 0.05 (tick size)
        result.newPrice = std::round(result.newPrice * 20.0) / 20.0;
        
        // CRITICAL: Ensure price actually moves (prevent stagnation at low prices)
        // If rounding caused no change, force a 1-tick move in the calculated direction
        if (result.newPrice == currentPrice) {
            result.newPrice = currentPrice + (moveResult.direction * 0.05);
        }
        
        // Enforce minimum price
        if (result.newPrice < 0.01) {
            result.newPrice = 0.01;
        }
        
        return result;
    }
    
    /**
     * Set anchor price for mean reversion
     */
    void setAnchorPrice(double price) {
        m_anchorPrice = price;
    }
    
    /**
     * Reset engine state
     */
    void reset() {
        m_consecutiveMoves = 0;
        m_lastDirection = 1;
        m_pullbackRemaining = 0;
        m_ticksSinceLastShock = 0;
        m_lastShockType = "";
        m_anchorPrice = 0.0;
    }
    
private:
    struct MoveResult {
        double change;
        int direction;
    };
    
    /**
     * Calculate normal (non-shock) price movement based on sentiment parameters
     */
    MoveResult calculateNormalMove(
        double currentPrice,
        const SentimentParams& params,
        double intensityMult,
        const std::string& sentiment
    ) {
        int direction;
        double effectiveUpProb = params.upProbability;
        
        // === MEAN REVERSION (for SIDEWAYS) ===
        if (params.meanReversion && m_anchorPrice > 0.0) {
            double deviation = (currentPrice - m_anchorPrice) / m_anchorPrice;
            // If price is above anchor, increase chance of going down
            // If price is below anchor, increase chance of going up
            double reversionStrength = 0.4; // How strongly to pull back
            effectiveUpProb = params.upProbability - (deviation * reversionStrength);
            // Clamp to reasonable range
            effectiveUpProb = std::max(0.2, std::min(0.8, effectiveUpProb));
        }
        
        // === CHOPPY: Random probability variation per tick ===
        if (sentiment == "CHOPPY") {
            // Add random noise to probability each tick
            effectiveUpProb = 0.35 + randomDouble() * 0.30; // Range: 35-65%
        }
        
        // === SUDDEN REVERSAL CHECK ===
        if (randomDouble() < params.reversalChance) {
            // Sudden direction change!
            direction = -m_lastDirection;
            m_consecutiveMoves = 1;
            m_lastDirection = direction;
        }
        // === FORCED PULLBACK (too many consecutive moves) ===
        else if (m_consecutiveMoves >= params.maxConsecutive) {
            // Force counter-trend move
            direction = -m_lastDirection;
            m_pullbackRemaining = 2 + randomInt(3); // 2-4 tick pullback
            m_consecutiveMoves = 0;
        }
        // === CONTINUING PULLBACK ===
        else if (m_pullbackRemaining > 0) {
            direction = -m_lastDirection;
            m_pullbackRemaining--;
            if (m_pullbackRemaining == 0) {
                m_consecutiveMoves = 0;
            }
        }
        // === NORMAL MOVE ===
        else {
            // Apply trend persistence
            double trendBonus = 0.0;
            if (m_consecutiveMoves > 0 && params.trendStrength > 0.5) {
                // If we've been going in one direction, trend strength makes us more likely to continue
                trendBonus = (params.trendStrength - 0.5) * 0.15; // Up to 7.5% bonus
                if (m_lastDirection < 0) {
                    effectiveUpProb -= trendBonus; // Continue down trend
                } else {
                    effectiveUpProb += trendBonus; // Continue up trend
                }
            }
            
            bool goUp = randomDouble() < effectiveUpProb;
            direction = goUp ? 1 : -1;
            
            if (direction == m_lastDirection) {
                m_consecutiveMoves++;
            } else {
                m_consecutiveMoves = 1;
                m_lastDirection = direction;
            }
        }
        
        // === CALCULATE MAGNITUDE ===
        // Base magnitude with some randomness
        double baseMagnitude = (0.5 + randomDouble() * 0.5) * params.baseVolatility;
        
        // Pullback moves are typically smaller
        double pullbackFactor = (m_pullbackRemaining > 0) ? 0.7 : 1.0;
        
        // Apply intensity
        double magnitude = baseMagnitude * intensityMult * pullbackFactor;
        
        // VOLATILE sentiment: occasionally spike the magnitude
        if (sentiment == "VOLATILE" && randomDouble() < 0.15) {
            magnitude *= 2.0; // Double-sized move 15% of the time
        }
        
        // CHOPPY sentiment: vary magnitude randomly
        if (sentiment == "CHOPPY") {
            magnitude *= (0.5 + randomDouble()); // 50-150% of normal
        }
        
        double change = direction * magnitude;
        return { change, direction };
    }
    
    double randomDouble() {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(m_rng);
    }
    
    int randomInt(int max) {
        if (max <= 0) return 0;
        std::uniform_int_distribution<int> dist(0, max - 1);
        return dist(m_rng);
    }
    
    // Trend tracking
    int m_consecutiveMoves = 0;
    int m_lastDirection = 1;
    int m_pullbackRemaining = 0;
    
    // Mean reversion anchor (for SIDEWAYS)
    double m_anchorPrice = 0.0;
    
    // News shock tracking
    int m_ticksSinceLastShock = 0;
    std::string m_lastShockType;
    
    // Random number generator
    std::mt19937 m_rng;
};

} // namespace orderbook

#endif // PRICE_ENGINE_H
