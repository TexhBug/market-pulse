#ifndef ORDERBOOK_H
#define ORDERBOOK_H

// ============================================================================
// ORDERBOOK.H - The Order Book class definition
// ============================================================================
// The OrderBook maintains all buy (bid) and sell (ask) orders.
// Think of it as the "marketplace" where all orders live.
// ============================================================================

#include "Common.h"
#include "Order.h"
#include <map>
#include <unordered_map>
#include <vector>
#include <list>
#include <mutex>
#include <optional>

namespace orderbook {

/**
 * @brief Represents all orders at a single price level
 * 
 * Multiple orders can exist at the same price. This class groups them
 * and maintains FIFO (first-in-first-out) order.
 */
struct PriceLevel {
    Price price = 0.0;
    Quantity totalQuantity = 0;
    std::list<Order*> orders;  // List for efficient insert/remove
    
    void addOrder(Order* order);
    void removeOrder(Order* order);
    bool isEmpty() const { return orders.empty(); }
};

/**
 * @brief The main Order Book class
 * 
 * Maintains:
 * - BIDS: Buy orders sorted by price (highest first)
 * - ASKS: Sell orders sorted by price (lowest first)
 * 
 * Thread-safe: All public methods are protected by mutex.
 * 
 * Example usage:
 *   OrderBook book;
 *   book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 50));
 *   auto bestBid = book.getBestBid();  // Returns 100.0
 */
class OrderBook {
public:
    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================
    
    OrderBook() = default;
    ~OrderBook();
    
    // Non-copyable (due to mutex and pointers)
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    
    // ========================================================================
    // ORDER MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Clear all orders from the book
     */
    void clear();
    
    /**
     * @brief Add a new order to the book
     * @param order The order to add
     * @return true if successfully added
     */
    bool addOrder(const Order& order);
    
    /**
     * @brief Cancel an order by ID
     * @param orderId The order to cancel
     * @return true if found and cancelled
     */
    bool cancelOrder(OrderId orderId);
    
    /**
     * @brief Modify an existing order's price
     * @param orderId Order to modify
     * @param newPrice New price
     * @return true if successful
     */
    bool modifyOrderPrice(OrderId orderId, Price newPrice);
    
    /**
     * @brief Modify an existing order's quantity
     * @param orderId Order to modify
     * @param newQuantity New quantity
     * @return true if successful
     */
    bool modifyOrderQuantity(OrderId orderId, Quantity newQuantity);
    
    /**
     * @brief Get an order by ID
     * @param orderId The order ID to find
     * @return Pointer to order if found, nullptr otherwise
     */
    Order* getOrder(OrderId orderId);
    
    // ========================================================================
    // MARKET DATA - Best prices and quantities
    // ========================================================================
    
    /**
     * @brief Get the best (highest) bid price
     * @return Best bid price, or std::nullopt if no bids
     */
    std::optional<Price> getBestBid() const;
    
    /**
     * @brief Get the best (lowest) ask price
     * @return Best ask price, or std::nullopt if no asks
     */
    std::optional<Price> getBestAsk() const;
    
    /**
     * @brief Get the spread (difference between best ask and best bid)
     * @return Spread, or std::nullopt if no bids or asks
     */
    std::optional<Price> getSpread() const;
    
    /**
     * @brief Get total quantity at a price level
     * @param side BUY or SELL
     * @param price The price level
     * @return Total quantity at that level
     */
    Quantity getQuantityAtPrice(Side side, Price price) const;
    
    /**
     * @brief Fill (reduce) quantity at a price level
     * Used by matching engine when orders are executed
     * @param side BUY or SELL
     * @param price The price level
     * @param quantity Amount to reduce
     * @return Actual quantity reduced (may be less if level didn't have enough)
     */
    Quantity fillQuantityAtPrice(Side side, Price price, Quantity quantity);
    
    // ========================================================================
    // BOOK SNAPSHOTS - For visualization
    // ========================================================================
    
    /**
     * @brief Get top N price levels for bids
     * @param n Number of levels to return
     * @return Vector of (price, quantity) pairs
     */
    std::vector<std::pair<Price, Quantity>> getTopBids(size_t n = 10) const;
    
    /**
     * @brief Get top N price levels for asks
     * @param n Number of levels to return
     * @return Vector of (price, quantity) pairs
     */
    std::vector<std::pair<Price, Quantity>> getTopAsks(size_t n = 10) const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t getBidLevelCount() const;
    size_t getAskLevelCount() const;
    size_t getTotalOrderCount() const;

private:
    // ========================================================================
    // INTERNAL DATA STRUCTURES
    // ========================================================================
    
    // Bids: sorted by price DESCENDING (highest first)
    // We use std::greater<Price> for descending order
    std::map<Price, PriceLevel, std::greater<Price>> m_bids;
    
    // Asks: sorted by price ASCENDING (lowest first)
    // Default std::map uses std::less (ascending)
    std::map<Price, PriceLevel> m_asks;
    
    // Fast lookup: OrderId -> Order pointer
    std::unordered_map<OrderId, Order*> m_orderMap;
    
    // Thread safety
    mutable std::mutex m_mutex;
    
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================
    
    void addToBook(Order* order);
    void removeFromBook(Order* order);
};

} // namespace orderbook

#endif // ORDERBOOK_H
