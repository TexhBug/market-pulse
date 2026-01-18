#ifndef VISUALIZER_H
#define VISUALIZER_H

// ============================================================================
// VISUALIZER.H - Order Book Visualization
// ============================================================================
// This class handles displaying the order book in the console.
// It creates a nice visual representation of bids and asks.
// ============================================================================

#include "Common.h"
#include "OrderBook.h"
#include "MarketSentiment.h"
#include <string>
#include <vector>
#include <atomic>

namespace orderbook {

/**
 * @brief Console-based visualizer for the order book
 * 
 * Displays:
 * - Top N bid and ask levels
 * - Best bid/ask and spread
 * - Order book statistics
 * - Recent trades
 * 
 * Example output:
 *   ╔══════════════════════════════════════════════════════╗
 *   ║           ORDER BOOK - AAPL                          ║
 *   ╠══════════════════════════════════════════════════════╣
 *   ║  BIDS (Buy)         │         ASKS (Sell)           ║
 *   ╠══════════════════════════════════════════════════════╣
 *   ║  $150.05  x  300 ← │ → $150.15  x  500              ║
 *   ║  $150.00  x  450   │   $150.20  x  250              ║
 *   ║  $149.95  x  200   │   $150.25  x  100              ║
 *   ╠══════════════════════════════════════════════════════╣
 *   ║  Spread: $0.10  |  Bid Levels: 3  |  Ask Levels: 3  ║
 *   ╚══════════════════════════════════════════════════════╝
 */
class Visualizer {
public:
    /**
     * @brief Create a visualizer for an order book
     * @param orderBook Reference to the order book to display
     * @param symbol The trading symbol (e.g., "AAPL")
     */
    Visualizer(const OrderBook& orderBook, const std::string& symbol = "STOCK");
    
    /**
     * @brief Set the sentiment controller for display
     * @param controller Pointer to sentiment controller (can be nullptr)
     */
    void setSentimentController(MarketSentimentController* controller) {
        m_sentimentController = controller;
    }
    
    // ========================================================================
    // DISPLAY METHODS
    // ========================================================================
    
    /**
     * @brief Render the order book to console
     * @param levels Number of price levels to show (default: 10)
     */
    void render(size_t levels = 10) const;
    
    /**
     * @brief Clear the console screen
     */
    void clearScreen() const;
    
    /**
     * @brief Print the header
     */
    void printHeader() const;
    
    /**
     * @brief Print the order book table
     * @param levels Number of levels to show
     */
    void printOrderBook(size_t levels = 10) const;
    
    /**
     * @brief Print statistics footer
     */
    void printFooter() const;
    
    // ========================================================================
    // TRADE DISPLAY
    // ========================================================================
    
    /**
     * @brief Add a trade to the recent trades list
     * @param price Trade price
     * @param quantity Trade quantity
     * @param side Side of the aggressor (taker)
     */
    void addTrade(Price price, Quantity quantity, Side side);
    
    /**
     * @brief Print recent trades
     * @param count Number of recent trades to show
     */
    void printRecentTrades(size_t count = 5) const;
    
    /**
     * @brief Print the current price ticker with change info
     */
    void printPriceTicker() const;
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    /**
     * @brief Set the column width for prices
     */
    void setPriceWidth(int width) { m_priceWidth = width; }
    
    /**
     * @brief Set the column width for quantities
     */
    void setQuantityWidth(int width) { m_quantityWidth = width; }
    
    /**
     * @brief Enable/disable colors (if terminal supports it)
     */
    void setColorEnabled(bool enabled) { m_colorEnabled = enabled; }
    
    /**
     * @brief Print ASCII price chart
     */
    void printPriceChart() const;

private:
    const OrderBook& m_orderBook;
    std::string m_symbol;
    MarketSentimentController* m_sentimentController = nullptr;
    
    // Recent trades for display
    struct TradeRecord {
        Price price;
        Quantity quantity;
        Side side;
    };
    mutable std::vector<TradeRecord> m_recentTrades;
    static constexpr size_t MAX_TRADE_HISTORY = 50;
    
    // Price history for chart
    mutable std::vector<Price> m_priceHistory;
    static constexpr size_t MAX_PRICE_HISTORY = 60;  // 60 data points for chart
    
    // Price tracking for change display
    mutable Price m_lastPrice = 0.0;
    mutable Price m_sessionOpenPrice = 0.0;
    mutable Price m_sessionHighPrice = 0.0;
    mutable Price m_sessionLowPrice = 999999.0;
    mutable Price m_previousPrice = 0.0;  // For tick direction
    
    // Display settings
    int m_priceWidth = 10;
    int m_quantityWidth = 8;
    bool m_colorEnabled = true;
    
    // ========================================================================
    // HELPERS
    // ========================================================================
    
    std::string formatPrice(Price price) const;
    std::string formatQuantity(Quantity qty) const;
    std::string colorize(const std::string& text, const std::string& color) const;
    void printLine(char c = '=', int width = 60) const;
    void updatePriceTracking(Price price) const;
};

} // namespace orderbook

#endif // VISUALIZER_H
