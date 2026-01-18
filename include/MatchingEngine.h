#ifndef MATCHINGENGINE_H
#define MATCHINGENGINE_H

// ============================================================================
// MATCHINGENGINE.H - The Matching Engine class definition
// ============================================================================
// The Matching Engine is the "matchmaker" - it pairs buy and sell orders.
// When a new order comes in, it checks if there's a compatible order on
// the other side and executes trades.
// ============================================================================

#include "Common.h"
#include "Order.h"
#include "OrderBook.h"
#include <vector>
#include <functional>

namespace orderbook {

/**
 * @brief Represents a single executed trade
 * 
 * When two orders match, we create a Trade record.
 */
struct Trade {
    OrderId buyOrderId;
    OrderId sellOrderId;
    Price   price;
    Quantity quantity;
    Timestamp timestamp;
    
    std::string toString() const;
};

/**
 * @brief Callback function type for trade notifications
 */
using TradeCallback = std::function<void(const Trade&)>;

/**
 * @brief The Matching Engine class
 * 
 * Responsibilities:
 * 1. Receive incoming orders
 * 2. Try to match them against existing orders
 * 3. Execute trades when matches are found
 * 4. Add unmatched orders to the book
 * 
 * Example:
 *   MatchingEngine engine(orderBook);
 *   engine.onTrade([](const Trade& t) { 
 *       std::cout << "Trade: " << t.toString() << std::endl; 
 *   });
 *   engine.processOrder(order);
 */
class MatchingEngine {
public:
    /**
     * @brief Create a matching engine for a specific order book
     * @param orderBook Reference to the order book to use
     */
    explicit MatchingEngine(OrderBook& orderBook);
    
    // ========================================================================
    // ORDER PROCESSING
    // ========================================================================
    
    /**
     * @brief Process an incoming order
     * 
     * This will:
     * 1. Try to match the order against the opposite side
     * 2. Execute any trades
     * 3. Add remaining quantity to the book (for LIMIT orders)
     * 
     * @param order The order to process
     * @return Vector of trades that occurred
     */
    std::vector<Trade> processOrder(Order& order);
    
    /**
     * @brief Cancel an existing order
     * @param orderId Order to cancel
     * @return true if cancelled successfully
     */
    bool cancelOrder(OrderId orderId);
    
    // ========================================================================
    // CALLBACKS
    // ========================================================================
    
    /**
     * @brief Register a callback for trade notifications
     * @param callback Function to call when a trade occurs
     */
    void onTrade(TradeCallback callback);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    /**
     * @brief Get total number of trades executed
     */
    size_t getTradeCount() const { return m_tradeCount; }
    
    /**
     * @brief Get total volume traded
     */
    Quantity getTotalVolume() const { return m_totalVolume; }

private:
    OrderBook& m_orderBook;
    std::vector<TradeCallback> m_tradeCallbacks;
    size_t m_tradeCount = 0;
    Quantity m_totalVolume = 0;
    
    // ========================================================================
    // INTERNAL MATCHING LOGIC
    // ========================================================================
    
    /**
     * @brief Try to match a BUY order against ASKs
     */
    std::vector<Trade> matchBuyOrder(Order& order);
    
    /**
     * @brief Try to match a SELL order against BIDs
     */
    std::vector<Trade> matchSellOrder(Order& order);
    
    /**
     * @brief Execute a trade between two orders
     */
    Trade executeTrade(Order& buyOrder, Order& sellOrder, Price price, Quantity qty);
    
    /**
     * @brief Notify all callbacks of a trade
     */
    void notifyTrade(const Trade& trade);
};

} // namespace orderbook

#endif // MATCHINGENGINE_H
