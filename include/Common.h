#ifndef COMMON_H
#define COMMON_H

// ============================================================================
// COMMON.H - Shared types and definitions
// ============================================================================
// This file contains common types used throughout the project.
// Think of it as the "dictionary" that everyone agrees on!
// ============================================================================

#include <cstdint>
#include <string>
#include <chrono>

namespace orderbook {

// ============================================================================
// TYPE ALIASES
// ============================================================================
// These make our code more readable and meaningful

using OrderId   = uint64_t;    // Unique identifier for each order
using Price     = double;       // Price in dollars (e.g., 150.25)
using Quantity  = uint32_t;     // Number of shares
using Timestamp = std::chrono::steady_clock::time_point;

// ============================================================================
// ENUMS
// ============================================================================

/**
 * @brief Side of the order - BUY or SELL
 * 
 * BUY (Bid)  = You want to purchase shares
 * SELL (Ask) = You want to sell shares
 */
enum class Side {
    BUY,    // Also called "Bid" - wanting to purchase
    SELL    // Also called "Ask" - wanting to sell
};

/**
 * @brief Type of order
 * 
 * LIMIT  = Execute at specified price or better
 * MARKET = Execute immediately at best available price
 */
enum class OrderType {
    LIMIT,  // Patient: "I want this price or better"
    MARKET  // Urgent: "I want it NOW, any price!"
};

/**
 * @brief Status of an order
 */
enum class OrderStatus {
    NEW,        // Just created, not yet processed
    OPEN,       // In the order book, waiting for a match
    FILLED,     // Completely executed
    PARTIAL,    // Partially filled, some quantity remaining
    CANCELLED,  // Cancelled by user
    REJECTED    // Rejected by the system
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Convert Side enum to string for display
 */
inline std::string sideToString(Side side) {
    return (side == Side::BUY) ? "BUY" : "SELL";
}

/**
 * @brief Convert OrderType enum to string for display
 */
inline std::string orderTypeToString(OrderType type) {
    return (type == OrderType::LIMIT) ? "LIMIT" : "MARKET";
}

/**
 * @brief Convert OrderStatus enum to string for display
 */
inline std::string statusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::NEW:       return "NEW";
        case OrderStatus::OPEN:      return "OPEN";
        case OrderStatus::FILLED:    return "FILLED";
        case OrderStatus::PARTIAL:   return "PARTIAL";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::REJECTED:  return "REJECTED";
        default:                     return "UNKNOWN";
    }
}

/**
 * @brief Get current timestamp
 */
inline Timestamp now() {
    return std::chrono::steady_clock::now();
}

} // namespace orderbook

#endif // COMMON_H
