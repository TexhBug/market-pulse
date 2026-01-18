#ifndef ORDERQUEUE_H
#define ORDERQUEUE_H

// ============================================================================
// ORDERQUEUE.H - Thread-safe Order Queue
// ============================================================================
// This is a thread-safe queue for passing orders between threads.
// Think of it as a "conveyor belt" that safely moves orders from
// the order generator to the order processor.
// ============================================================================

#include "Order.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <atomic>

namespace orderbook {

/**
 * @brief A thread-safe queue for orders
 * 
 * Features:
 * - Multiple producers can push orders simultaneously
 * - Multiple consumers can pop orders simultaneously
 * - Blocking wait when queue is empty
 * - Graceful shutdown support
 * 
 * Example:
 *   OrderQueue queue;
 *   
 *   // Producer thread
 *   queue.push(Order(...));
 *   
 *   // Consumer thread
 *   auto order = queue.pop();  // Blocks if empty
 *   if (order) {
 *       process(*order);
 *   }
 */
class OrderQueue {
public:
    OrderQueue() = default;
    ~OrderQueue() = default;
    
    // Non-copyable
    OrderQueue(const OrderQueue&) = delete;
    OrderQueue& operator=(const OrderQueue&) = delete;
    
    // ========================================================================
    // PRODUCER METHODS
    // ========================================================================
    
    /**
     * @brief Add an order to the queue
     * @param order The order to add
     */
    void push(const Order& order);
    
    /**
     * @brief Add an order to the queue (move version)
     * @param order The order to add (will be moved)
     */
    void push(Order&& order);
    
    // ========================================================================
    // CONSUMER METHODS
    // ========================================================================
    
    /**
     * @brief Remove and return the next order (blocking)
     * 
     * This will WAIT if the queue is empty, until:
     * - An order is available, OR
     * - shutdown() is called
     * 
     * @return The next order, or std::nullopt if shutting down
     */
    std::optional<Order> pop();
    
    /**
     * @brief Try to remove an order without waiting
     * @return The next order, or std::nullopt if queue is empty
     */
    std::optional<Order> tryPop();
    
    /**
     * @brief Wait for an order with a timeout
     * @param timeoutMs Maximum time to wait in milliseconds
     * @return The next order, or std::nullopt if timeout/shutdown
     */
    std::optional<Order> popWithTimeout(int timeoutMs);
    
    // ========================================================================
    // QUEUE STATUS
    // ========================================================================
    
    /**
     * @brief Check if queue is empty
     */
    bool empty() const;
    
    /**
     * @brief Get number of orders in queue
     */
    size_t size() const;
    
    /**
     * @brief Clear all orders from the queue
     */
    void clear();
    
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
    /**
     * @brief Signal shutdown - unblocks all waiting consumers
     * 
     * After calling this, pop() will return std::nullopt instead of waiting.
     */
    void shutdown();
    
    /**
     * @brief Check if shutdown was requested
     */
    bool isShutdown() const { return m_shutdown.load(); }

private:
    std::queue<Order> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_shutdown{false};
};

} // namespace orderbook

#endif // ORDERQUEUE_H
