// ============================================================================
// ORDERBOOK.CPP - OrderBook class implementation
// ============================================================================

#include "OrderBook.h"
#include <algorithm>

namespace orderbook {

// ============================================================================
// PRICE LEVEL METHODS
// ============================================================================

void PriceLevel::addOrder(Order* order) {
    orders.push_back(order);
    totalQuantity += order->getRemainingQty();
}

void PriceLevel::removeOrder(Order* order) {
    auto it = std::find(orders.begin(), orders.end(), order);
    if (it != orders.end()) {
        totalQuantity -= (*it)->getRemainingQty();
        orders.erase(it);
    }
}

// ============================================================================
// ORDERBOOK DESTRUCTOR
// ============================================================================

OrderBook::~OrderBook() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clean up all allocated orders
    for (auto& [id, order] : m_orderMap) {
        delete order;
    }
    m_orderMap.clear();
    m_bids.clear();
    m_asks.clear();
}

// ============================================================================
// CLEAR ALL ORDERS
// ============================================================================

void OrderBook::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clean up all allocated orders
    for (auto& [id, order] : m_orderMap) {
        delete order;
    }
    m_orderMap.clear();
    m_bids.clear();
    m_asks.clear();
}

// ============================================================================
// ORDER MANAGEMENT
// ============================================================================

bool OrderBook::addOrder(const Order& order) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if order ID already exists
    if (m_orderMap.find(order.getId()) != m_orderMap.end()) {
        return false;  // Duplicate order ID
    }
    
    // Create a copy of the order on the heap
    Order* newOrder = new Order(order);
    newOrder->getStatus();  // Trigger status update if needed
    
    // Add to lookup map
    m_orderMap[newOrder->getId()] = newOrder;
    
    // Add to the appropriate book (bids or asks)
    addToBook(newOrder);
    
    return true;
}

bool OrderBook::cancelOrder(OrderId orderId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_orderMap.find(orderId);
    if (it == m_orderMap.end()) {
        return false;  // Order not found
    }
    
    Order* order = it->second;
    
    if (!order->isActive()) {
        return false;  // Already cancelled or filled
    }
    
    // Remove from book
    removeFromBook(order);
    
    // Mark as cancelled
    order->cancel();
    
    return true;
}

bool OrderBook::modifyOrderPrice(OrderId orderId, Price newPrice) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_orderMap.find(orderId);
    if (it == m_orderMap.end()) {
        return false;
    }
    
    Order* order = it->second;
    
    if (!order->isActive()) {
        return false;
    }
    
    // Remove from current price level
    removeFromBook(order);
    
    // Modify price
    if (!order->modifyPrice(newPrice)) {
        // If modify failed, add back to original location
        addToBook(order);
        return false;
    }
    
    // Add to new price level
    addToBook(order);
    
    return true;
}

bool OrderBook::modifyOrderQuantity(OrderId orderId, Quantity newQuantity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_orderMap.find(orderId);
    if (it == m_orderMap.end()) {
        return false;
    }
    
    Order* order = it->second;
    
    if (!order->isActive()) {
        return false;
    }
    
    // Get old remaining quantity
    Quantity oldRemaining = order->getRemainingQty();
    
    // Modify quantity
    if (!order->modifyQuantity(newQuantity)) {
        return false;
    }
    
    // Update the price level's total quantity
    Quantity newRemaining = order->getRemainingQty();
    Quantity diff = newRemaining - oldRemaining;
    
    Price price = order->getPrice();
    if (order->getSide() == Side::BUY) {
        if (m_bids.count(price)) {
            m_bids[price].totalQuantity += diff;
        }
    } else {
        if (m_asks.count(price)) {
            m_asks[price].totalQuantity += diff;
        }
    }
    
    return true;
}

Order* OrderBook::getOrder(OrderId orderId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_orderMap.find(orderId);
    if (it != m_orderMap.end()) {
        return it->second;
    }
    return nullptr;
}

// ============================================================================
// MARKET DATA
// ============================================================================

std::optional<Price> OrderBook::getBestBid() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_bids.empty()) {
        return std::nullopt;
    }
    return m_bids.begin()->first;  // Highest price (sorted descending)
}

std::optional<Price> OrderBook::getBestAsk() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_asks.empty()) {
        return std::nullopt;
    }
    return m_asks.begin()->first;  // Lowest price (sorted ascending)
}

std::optional<Price> OrderBook::getSpread() const {
    auto bid = getBestBid();
    auto ask = getBestAsk();
    
    if (bid && ask) {
        return *ask - *bid;
    }
    return std::nullopt;
}

Quantity OrderBook::getQuantityAtPrice(Side side, Price price) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (side == Side::BUY) {
        auto it = m_bids.find(price);
        if (it != m_bids.end()) {
            return it->second.totalQuantity;
        }
    } else {
        auto it = m_asks.find(price);
        if (it != m_asks.end()) {
            return it->second.totalQuantity;
        }
    }
    return 0;
}

Quantity OrderBook::fillQuantityAtPrice(Side side, Price price, Quantity quantity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Quantity filled = 0;
    
    if (side == Side::BUY) {
        auto it = m_bids.find(price);
        if (it == m_bids.end()) return 0;
        
        PriceLevel& level = it->second;
        
        // Fill orders FIFO until quantity satisfied or level exhausted
        while (!level.orders.empty() && filled < quantity) {
            Order* order = level.orders.front();
            Quantity orderRemaining = order->getRemainingQty();
            Quantity toFill = std::min(orderRemaining, quantity - filled);
            
            order->fill(toFill);
            level.totalQuantity -= toFill;
            filled += toFill;
            
            // Remove fully filled orders
            if (order->getRemainingQty() == 0) {
                level.orders.pop_front();
                m_orderMap.erase(order->getId());
                delete order;
            }
        }
        
        // Remove empty price level
        if (level.totalQuantity == 0 || level.orders.empty()) {
            m_bids.erase(it);
        }
    } else {
        auto it = m_asks.find(price);
        if (it == m_asks.end()) return 0;
        
        PriceLevel& level = it->second;
        
        // Fill orders FIFO until quantity satisfied or level exhausted
        while (!level.orders.empty() && filled < quantity) {
            Order* order = level.orders.front();
            Quantity orderRemaining = order->getRemainingQty();
            Quantity toFill = std::min(orderRemaining, quantity - filled);
            
            order->fill(toFill);
            level.totalQuantity -= toFill;
            filled += toFill;
            
            // Remove fully filled orders
            if (order->getRemainingQty() == 0) {
                level.orders.pop_front();
                m_orderMap.erase(order->getId());
                delete order;
            }
        }
        
        // Remove empty price level
        if (level.totalQuantity == 0 || level.orders.empty()) {
            m_asks.erase(it);
        }
    }
    
    return filled;
}

// ============================================================================
// BOOK SNAPSHOTS
// ============================================================================

std::vector<std::pair<Price, Quantity>> OrderBook::getTopBids(size_t n) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::pair<Price, Quantity>> result;
    result.reserve(n);
    
    size_t count = 0;
    for (const auto& [price, level] : m_bids) {
        if (count >= n) break;
        result.emplace_back(price, level.totalQuantity);
        ++count;
    }
    
    return result;
}

std::vector<std::pair<Price, Quantity>> OrderBook::getTopAsks(size_t n) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::pair<Price, Quantity>> result;
    result.reserve(n);
    
    size_t count = 0;
    for (const auto& [price, level] : m_asks) {
        if (count >= n) break;
        result.emplace_back(price, level.totalQuantity);
        ++count;
    }
    
    return result;
}

// ============================================================================
// STATISTICS
// ============================================================================

size_t OrderBook::getBidLevelCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bids.size();
}

size_t OrderBook::getAskLevelCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_asks.size();
}

size_t OrderBook::getTotalOrderCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_orderMap.size();
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

void OrderBook::addToBook(Order* order) {
    Price price = order->getPrice();
    
    if (order->getSide() == Side::BUY) {
        // Add to bids
        if (m_bids.find(price) == m_bids.end()) {
            m_bids[price] = PriceLevel{price, 0, {}};
        }
        m_bids[price].addOrder(order);
    } else {
        // Add to asks
        if (m_asks.find(price) == m_asks.end()) {
            m_asks[price] = PriceLevel{price, 0, {}};
        }
        m_asks[price].addOrder(order);
    }
}

void OrderBook::removeFromBook(Order* order) {
    Price price = order->getPrice();
    
    if (order->getSide() == Side::BUY) {
        auto it = m_bids.find(price);
        if (it != m_bids.end()) {
            it->second.removeOrder(order);
            if (it->second.isEmpty()) {
                m_bids.erase(it);
            }
        }
    } else {
        auto it = m_asks.find(price);
        if (it != m_asks.end()) {
            it->second.removeOrder(order);
            if (it->second.isEmpty()) {
                m_asks.erase(it);
            }
        }
    }
}

} // namespace orderbook
