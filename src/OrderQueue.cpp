// ============================================================================
// ORDERQUEUE.CPP - Thread-safe Order Queue implementation
// ============================================================================

#include "OrderQueue.h"

namespace orderbook {

// ============================================================================
// PRODUCER METHODS
// ============================================================================

void OrderQueue::push(const Order& order) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(order);
    }
    // Notify one waiting consumer
    m_condition.notify_one();
}

void OrderQueue::push(Order&& order) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(order));
    }
    m_condition.notify_one();
}

// ============================================================================
// CONSUMER METHODS
// ============================================================================

std::optional<Order> OrderQueue::pop() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // Wait until queue is not empty OR shutdown is requested
    m_condition.wait(lock, [this] {
        return !m_queue.empty() || m_shutdown.load();
    });
    
    // If shutdown and queue is empty, return nothing
    if (m_shutdown.load() && m_queue.empty()) {
        return std::nullopt;
    }
    
    // Get the front order
    Order order = std::move(m_queue.front());
    m_queue.pop();
    
    return order;
}

std::optional<Order> OrderQueue::tryPop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_queue.empty()) {
        return std::nullopt;
    }
    
    Order order = std::move(m_queue.front());
    m_queue.pop();
    
    return order;
}

std::optional<Order> OrderQueue::popWithTimeout(int timeoutMs) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // Wait with timeout
    bool gotItem = m_condition.wait_for(
        lock, 
        std::chrono::milliseconds(timeoutMs),
        [this] { return !m_queue.empty() || m_shutdown.load(); }
    );
    
    // If timeout expired or shutdown with empty queue
    if (!gotItem || (m_shutdown.load() && m_queue.empty())) {
        return std::nullopt;
    }
    
    Order order = std::move(m_queue.front());
    m_queue.pop();
    
    return order;
}

// ============================================================================
// QUEUE STATUS
// ============================================================================

bool OrderQueue::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

size_t OrderQueue::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void OrderQueue::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<Order> empty;
    std::swap(m_queue, empty);  // Clear by swapping with empty queue
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void OrderQueue::shutdown() {
    m_shutdown.store(true);
    // Wake up all waiting consumers
    m_condition.notify_all();
}

} // namespace orderbook
