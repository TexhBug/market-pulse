#ifndef ORDER_H
#define ORDER_H

// ============================================================================
// ORDER.H - The Order class definition
// ============================================================================
// An Order represents a single buy or sell request.
// Think of it as a "ticket" with all the details of what someone wants to do.
// ============================================================================

#include "Common.h"
#include <string>

namespace orderbook {

/**
 * @brief Represents a single order in the order book
 * 
 * An order contains:
 * - WHO: orderId (unique identifier)
 * - WHAT: side (BUY or SELL), type (LIMIT or MARKET)
 * - HOW MUCH: price and quantity
 * - WHEN: timestamp
 * - STATUS: current state of the order
 * 
 * Example:
 *   Order order(1, Side::BUY, OrderType::LIMIT, 150.50, 100);
 *   // "Order #1: BUY 100 shares at $150.50 (LIMIT)"
 */
class Order {
public:
    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================
    
    /**
     * @brief Default constructor - creates an empty order
     */
    Order() = default;
    
    /**
     * @brief Create a new order with all details
     * 
     * @param id       Unique order ID
     * @param side     BUY or SELL
     * @param type     LIMIT or MARKET
     * @param price    Price per share (ignored for MARKET orders)
     * @param quantity Number of shares
     */
    Order(OrderId id, Side side, OrderType type, Price price, Quantity quantity);
    
    // ========================================================================
    // GETTERS - Read order information
    // ========================================================================
    
    OrderId     getId()        const { return m_id; }
    Side        getSide()      const { return m_side; }
    OrderType   getType()      const { return m_type; }
    Price       getPrice()     const { return m_price; }
    Quantity    getQuantity()  const { return m_quantity; }
    Quantity    getFilledQty() const { return m_filledQty; }
    Quantity    getRemainingQty() const { return m_quantity - m_filledQty; }
    OrderStatus getStatus()    const { return m_status; }
    Timestamp   getTimestamp() const { return m_timestamp; }
    
    // ========================================================================
    // SETTERS / MODIFIERS
    // ========================================================================
    
    /**
     * @brief Fill some quantity of this order
     * @param qty Amount to fill
     * @return true if successful, false if trying to fill more than remaining
     */
    bool fill(Quantity qty);
    
    /**
     * @brief Cancel this order
     */
    void cancel();
    
    /**
     * @brief Modify the price (only for LIMIT orders that haven't been filled)
     * @param newPrice New price
     * @return true if successful
     */
    bool modifyPrice(Price newPrice);
    
    /**
     * @brief Modify the quantity (can only decrease for partially filled orders)
     * @param newQuantity New quantity
     * @return true if successful
     */
    bool modifyQuantity(Quantity newQuantity);
    
    // ========================================================================
    // UTILITY METHODS
    // ========================================================================
    
    /**
     * @brief Check if order is still active (can be matched)
     */
    bool isActive() const;
    
    /**
     * @brief Check if order is completely filled
     */
    bool isFilled() const { return m_status == OrderStatus::FILLED; }
    
    /**
     * @brief Get a string representation for display
     */
    std::string toString() const;

private:
    OrderId     m_id        = 0;
    Side        m_side      = Side::BUY;
    OrderType   m_type      = OrderType::LIMIT;
    Price       m_price     = 0.0;
    Quantity    m_quantity  = 0;
    Quantity    m_filledQty = 0;
    OrderStatus m_status    = OrderStatus::NEW;
    Timestamp   m_timestamp = now();
};

} // namespace orderbook

#endif // ORDER_H
