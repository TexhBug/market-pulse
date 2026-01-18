// ============================================================================
// CANDLEMANAGER.H - Candlestick Data Management (ported from Node.js)
// ============================================================================
// Handles OHLCV candle aggregation for multiple timeframes
// ============================================================================

#ifndef CANDLE_MANAGER_H
#define CANDLE_MANAGER_H

#include <map>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace orderbook {

// ============================================================================
// Configuration
// ============================================================================

// Timeframes in seconds (matching Node.js config.js)
const std::vector<int> TIMEFRAMES = { 1, 5, 30, 60, 300 };
constexpr int MAX_CACHED_CANDLES = 500;

// ============================================================================
// Candle Structure
// ============================================================================

struct Candle {
    int64_t timestamp;  // Period start timestamp (ms)
    double open;
    double high;
    double low;
    double close;
    int volume;
    
    Candle() : timestamp(0), open(0), high(0), low(0), close(0), volume(0) {}
    
    Candle(int64_t ts, double price, int vol)
        : timestamp(ts), open(price), high(price), low(price), close(price), volume(vol) {}
};

// ============================================================================
// Completed Candle (with timeframe info)
// ============================================================================

struct CompletedCandle {
    int timeframe;  // in seconds
    Candle candle;
};

// ============================================================================
// Candle Manager Class
// ============================================================================

class CandleManager {
public:
    CandleManager() {
        // Initialize for each timeframe
        for (int tf : TIMEFRAMES) {
            m_candleCache[tf] = std::vector<Candle>();
            m_currentCandles[tf] = Candle();
            m_hasCurrentCandle[tf] = false;
        }
    }
    
    /**
     * Update candles with a new price tick
     * @param price - Current price
     * @param volume - Tick volume
     * @param timestamp - Tick timestamp (ms)
     * @returns Vector of completed candles
     */
    std::vector<CompletedCandle> updateCandles(double price, int volume, int64_t timestamp) {
        std::vector<CompletedCandle> completedCandles;
        
        for (int tf : TIMEFRAMES) {
            int64_t tfMs = tf * 1000LL;
            int64_t periodStart = (timestamp / tfMs) * tfMs;
            
            bool hasCandle = m_hasCurrentCandle[tf];
            Candle& currentCandle = m_currentCandles[tf];
            
            if (!hasCandle || currentCandle.timestamp != periodStart) {
                // New period - save old candle if exists
                if (hasCandle) {
                    m_candleCache[tf].push_back(currentCandle);
                    
                    // Trim to max cached candles
                    if (m_candleCache[tf].size() > MAX_CACHED_CANDLES) {
                        m_candleCache[tf].erase(m_candleCache[tf].begin());
                    }
                    
                    completedCandles.push_back({ tf, currentCandle });
                }
                
                // Start new candle
                m_currentCandles[tf] = Candle(periodStart, price, volume);
                m_hasCurrentCandle[tf] = true;
            } else {
                // Update existing candle
                currentCandle.high = std::max(currentCandle.high, price);
                currentCandle.low = std::min(currentCandle.low, price);
                currentCandle.close = price;
                currentCandle.volume += volume;
            }
        }
        
        return completedCandles;
    }
    
    /**
     * Get cached candles for a specific timeframe
     */
    const std::vector<Candle>& getCachedCandles(int timeframe) const {
        static std::vector<Candle> empty;
        auto it = m_candleCache.find(timeframe);
        return it != m_candleCache.end() ? it->second : empty;
    }
    
    /**
     * Get current (incomplete) candle for a timeframe
     */
    const Candle* getCurrentCandle(int timeframe) const {
        auto it = m_hasCurrentCandle.find(timeframe);
        if (it != m_hasCurrentCandle.end() && it->second) {
            auto candleIt = m_currentCandles.find(timeframe);
            if (candleIt != m_currentCandles.end()) {
                return &candleIt->second;
            }
        }
        return nullptr;
    }
    
    /**
     * Get all current candles as a map
     */
    std::map<int, Candle> getCurrentCandles() const {
        std::map<int, Candle> result;
        for (int tf : TIMEFRAMES) {
            auto it = m_hasCurrentCandle.find(tf);
            if (it != m_hasCurrentCandle.end() && it->second) {
                auto candleIt = m_currentCandles.find(tf);
                if (candleIt != m_currentCandles.end()) {
                    result[tf] = candleIt->second;
                }
            }
        }
        return result;
    }
    
    /**
     * Reset all candle data
     */
    void reset() {
        for (int tf : TIMEFRAMES) {
            m_candleCache[tf].clear();
            m_currentCandles[tf] = Candle();
            m_hasCurrentCandle[tf] = false;
        }
    }
    
private:
    // Candle cache: stores completed candles for each timeframe
    std::map<int, std::vector<Candle>> m_candleCache;
    
    // Current (incomplete) candle for each timeframe
    mutable std::map<int, Candle> m_currentCandles;
    
    // Track if we have a current candle for each timeframe
    std::map<int, bool> m_hasCurrentCandle;
};

} // namespace orderbook

#endif // CANDLE_MANAGER_H
