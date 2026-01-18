// ============================================================================
// NEWS_SHOCK.H - News Shock Feature for Sudden Market Events
// ============================================================================
// Simulates sudden market-moving events like earnings, Fed decisions, etc.
// Shock is INDEPENDENT of normal volatility - always causes visible price move
// ============================================================================

#ifndef NEWS_SHOCK_H
#define NEWS_SHOCK_H

#include <random>
#include <atomic>
#include <chrono>
#include <string>

namespace orderbook {

// ============================================================================
// NEWS SHOCK CONFIGURATION
// ============================================================================

struct NewsShockConfig {
    // Timing
    static constexpr int ACTIVE_DURATION_MS = 5000;      // 5 seconds active
    static constexpr int COOLDOWN_DURATION_MS = 20000;   // 20 seconds cooldown
    static constexpr int MIN_TICKS_BETWEEN_SHOCKS = 20;  // ~2 seconds min
    
    // Probability
    static constexpr double TRIGGER_CHANCE = 0.03;       // 3% per tick
    
    // Impact (fixed percentage, independent of volatility)
    static constexpr double MIN_SHOCK_PERCENT = 0.01;    // 1% minimum
    static constexpr double MAX_SHOCK_PERCENT = 0.03;    // 3% maximum
};

// ============================================================================
// NEWS SHOCK CONTROLLER
// ============================================================================

class NewsShockController {
public:
    NewsShockController() 
        : m_enabled(false)
        , m_ticksSinceLastShock(0)
        , m_activeUntil(0)
        , m_cooldownUntil(0)
        , m_gen(std::random_device{}())
    {}
    
    // Enable/disable shock window
    bool enable() {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        
        // Check cooldown
        if (nowMs < m_cooldownUntil) {
            return false;  // Still in cooldown
        }
        
        m_enabled = true;
        m_activeUntil = nowMs + NewsShockConfig::ACTIVE_DURATION_MS;
        return true;
    }
    
    void disable() {
        if (m_enabled) {
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
            m_cooldownUntil = nowMs + NewsShockConfig::COOLDOWN_DURATION_MS;
        }
        m_enabled = false;
    }
    
    // Check if active window has expired
    void checkExpiration() {
        if (!m_enabled) return;
        
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        
        if (nowMs >= m_activeUntil) {
            disable();  // Auto-disable and start cooldown
        }
    }
    
    // State getters
    bool isEnabled() const { return m_enabled; }
    
    bool isInCooldown() const {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        return nowMs < m_cooldownUntil;
    }
    
    int getCooldownRemaining() const {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        if (nowMs >= m_cooldownUntil) return 0;
        return static_cast<int>((m_cooldownUntil - nowMs) / 1000);
    }
    
    int getActiveRemaining() const {
        if (!m_enabled) return 0;
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        if (nowMs >= m_activeUntil) return 0;
        return static_cast<int>((m_activeUntil - nowMs) / 1000);
    }
    
    // ========================================================================
    // APPLY SHOCK - Returns shock multiplier if shock triggers, else 0
    // ========================================================================
    struct ShockResult {
        bool applied;
        double priceMultiplier;  // 1.0 + direction * shockPercent
        std::string type;        // "bullish" or "bearish"
    };
    
    ShockResult tryApplyShock() {
        ShockResult result{false, 1.0, ""};
        
        if (!m_enabled) return result;
        
        m_ticksSinceLastShock++;
        
        // Check minimum ticks between shocks
        if (m_ticksSinceLastShock < NewsShockConfig::MIN_TICKS_BETWEEN_SHOCKS) {
            return result;
        }
        
        // Random chance
        std::uniform_real_distribution<> prob(0.0, 1.0);
        if (prob(m_gen) >= NewsShockConfig::TRIGGER_CHANCE) {
            return result;
        }
        
        // SHOCK EVENT!
        result.applied = true;
        
        // Random direction (50/50)
        int direction = prob(m_gen) < 0.5 ? 1 : -1;
        result.type = direction > 0 ? "bullish" : "bearish";
        
        // Fixed shock percentage: 1-3%
        std::uniform_real_distribution<> shockDist(
            NewsShockConfig::MIN_SHOCK_PERCENT,
            NewsShockConfig::MAX_SHOCK_PERCENT
        );
        double shockPercent = shockDist(m_gen);
        
        result.priceMultiplier = 1.0 + direction * shockPercent;
        
        m_ticksSinceLastShock = 0;
        
        return result;
    }
    
    void reset() {
        m_enabled = false;
        m_ticksSinceLastShock = 0;
        m_activeUntil = 0;
        m_cooldownUntil = 0;
    }

private:
    std::atomic<bool> m_enabled;
    int m_ticksSinceLastShock;
    int64_t m_activeUntil;
    int64_t m_cooldownUntil;
    std::mt19937 m_gen;
};

} // namespace orderbook

#endif // NEWS_SHOCK_H
