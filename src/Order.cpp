// ============================================================================
// ORDER.CPP - Order class implementation
// ============================================================================

#include "Order.h"
#include <sstream>
#include <iomanip>

namespace orderbook {

// ============================================================================
// CONSTRUCTOR
// ============================================================================

Order::Order(OrderId id, Side side, OrderType type, Price price, Quantity quantity)
    : m_id(id)
    , m_side(side)
    , m_type(type)
    , m_price(price)
    , m_quantity(quantity)
    , m_filledQty(0)
    , m_status(OrderStatus::NEW)
    , m_timestamp(now())
{
}

// ============================================================================
// FILL / CANCEL / MODIFY
// ============================================================================

bool Order::fill(Quantity qty) {
    // Can't fill more than remaining
    if (qty > getRemainingQty()) {
        return false;
    }
    
    m_filledQty += qty;
    
    // Update status
    if (m_filledQty == m_quantity) {
        m_status = OrderStatus::FILLED;
    } else if (m_filledQty > 0) {
        m_status = OrderStatus::PARTIAL;
    }
    
    return true;
}

void Order::cancel() {
    if (isActive()) {
        m_status = OrderStatus::CANCELLED;
    }
}

bool Order::modifyPrice(Price newPrice) {
    // Can only modify LIMIT orders that haven't been filled
    if (m_type != OrderType::LIMIT || m_filledQty > 0) {
        return false;
    }
    
    m_price = newPrice;
    return true;
}

bool Order::modifyQuantity(Quantity newQuantity) {
    // New quantity must be at least as much as already filled
    if (newQuantity < m_filledQty) {
        return false;
    }
    
    m_quantity = newQuantity;
    
    // Check if now fully filled
    if (m_filledQty == m_quantity) {
        m_status = OrderStatus::FILLED;
    }
    
    return true;
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

bool Order::isActive() const {
    return m_status == OrderStatus::NEW || 
           m_status == OrderStatus::OPEN || 
           m_status == OrderStatus::PARTIAL;
}

std::string Order::toString() const {
    std::ostringstream oss;
    oss << "Order #" << m_id << ": "
        << sideToString(m_side) << " "
        << m_quantity << " @ $"
        << std::fixed << std::setprecision(2) << m_price
        << " (" << orderTypeToString(m_type) << ")"
        << " [" << statusToString(m_status) << "]";
    
    if (m_filledQty > 0) {
        oss << " Filled: " << m_filledQty << "/" << m_quantity;
    }
    
    return oss.str();
}

} // namespace orderbook
