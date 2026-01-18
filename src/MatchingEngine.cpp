// ============================================================================
// MATCHINGENGINE.CPP - Matching Engine implementation
// ============================================================================

#include "MatchingEngine.h"
#include <sstream>
#include <iomanip>

namespace orderbook {

// ============================================================================
// TRADE TO STRING
// ============================================================================

std::string Trade::toString() const {
    std::ostringstream oss;
    oss << "Trade: Buy#" << buyOrderId 
        << " x Sell#" << sellOrderId
        << " | " << quantity << " @ $"
        << std::fixed << std::setprecision(2) << price;
    return oss.str();
}

// ============================================================================
// CONSTRUCTOR
// ============================================================================

MatchingEngine::MatchingEngine(OrderBook& orderBook)
    : m_orderBook(orderBook)
{
}

// ============================================================================
// ORDER PROCESSING
// ============================================================================

std::vector<Trade> MatchingEngine::processOrder(Order& order) {
    std::vector<Trade> trades;
    
    // Try to match the order
    if (order.getSide() == Side::BUY) {
        trades = matchBuyOrder(order);
    } else {
        trades = matchSellOrder(order);
    }
    
    // If there's remaining quantity and it's a LIMIT order, add to book
    if (order.getRemainingQty() > 0 && order.getType() == OrderType::LIMIT) {
        m_orderBook.addOrder(order);
    }
    
    return trades;
}

bool MatchingEngine::cancelOrder(OrderId orderId) {
    return m_orderBook.cancelOrder(orderId);
}

// ============================================================================
// CALLBACKS
// ============================================================================

void MatchingEngine::onTrade(TradeCallback callback) {
    m_tradeCallbacks.push_back(callback);
}

// ============================================================================
// INTERNAL MATCHING LOGIC
// ============================================================================

std::vector<Trade> MatchingEngine::matchBuyOrder(Order& order) {
    std::vector<Trade> trades;
    
    // Get the best asks (lowest prices) - these will be matched against
    auto asks = m_orderBook.getTopAsks(100);  // Get up to 100 levels
    
    for (const auto& [askPrice, askQty] : asks) {
        // Stop if order is filled
        if (order.getRemainingQty() == 0) break;
        
        // For LIMIT orders, stop if ask price is higher than our bid
        if (order.getType() == OrderType::LIMIT && askPrice > order.getPrice()) {
            break;
        }
        
        // Match at this price level
        Quantity matchQty = std::min(order.getRemainingQty(), askQty);
        
        // CRITICAL: Actually remove the quantity from the order book!
        Quantity actualFilled = m_orderBook.fillQuantityAtPrice(Side::SELL, askPrice, matchQty);
        if (actualFilled == 0) continue;
        
        // Fill the incoming order
        order.fill(actualFilled);
        
        // Create trade record
        Trade trade;
        trade.buyOrderId = order.getId();
        trade.sellOrderId = 0;  // Synthetic - matched against book
        trade.price = askPrice;
        trade.quantity = actualFilled;
        trade.timestamp = now();
        trades.push_back(trade);
        
        // Update statistics
        m_tradeCount++;
        m_totalVolume += actualFilled;
        
        // Notify callbacks
        notifyTrade(trade);
    }
    
    return trades;
}

std::vector<Trade> MatchingEngine::matchSellOrder(Order& order) {
    std::vector<Trade> trades;
    
    // Get the best bids (highest prices) - these will be matched against
    auto bids = m_orderBook.getTopBids(100);  // Get up to 100 levels
    
    for (const auto& [bidPrice, bidQty] : bids) {
        // Stop if order is filled
        if (order.getRemainingQty() == 0) break;
        
        // For LIMIT orders, stop if bid price is lower than our ask
        if (order.getType() == OrderType::LIMIT && bidPrice < order.getPrice()) {
            break;
        }
        
        // Match at this price level
        Quantity matchQty = std::min(order.getRemainingQty(), bidQty);
        
        // CRITICAL: Actually remove the quantity from the order book!
        Quantity actualFilled = m_orderBook.fillQuantityAtPrice(Side::BUY, bidPrice, matchQty);
        if (actualFilled == 0) continue;
        
        // Fill the incoming order
        order.fill(actualFilled);
        
        // Create trade record
        Trade trade;
        trade.buyOrderId = 0;  // Synthetic - matched against book
        trade.sellOrderId = order.getId();
        trade.price = bidPrice;
        trade.quantity = actualFilled;
        trade.timestamp = now();
        trades.push_back(trade);
        
        // Update statistics
        m_tradeCount++;
        m_totalVolume += actualFilled;
        
        // Notify callbacks
        notifyTrade(trade);
    }
    
    return trades;
}

Trade MatchingEngine::executeTrade(Order& buyOrder, Order& sellOrder, 
                                    Price price, Quantity qty) {
    // Fill both orders
    buyOrder.fill(qty);
    sellOrder.fill(qty);
    
    // Create trade record
    Trade trade;
    trade.buyOrderId = buyOrder.getId();
    trade.sellOrderId = sellOrder.getId();
    trade.price = price;
    trade.quantity = qty;
    trade.timestamp = now();
    
    return trade;
}

void MatchingEngine::notifyTrade(const Trade& trade) {
    for (const auto& callback : m_tradeCallbacks) {
        callback(trade);
    }
}

} // namespace orderbook
