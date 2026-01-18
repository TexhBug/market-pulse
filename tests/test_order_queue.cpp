// ============================================================================
// TEST_ORDER_QUEUE.CPP - Unit tests for OrderQueue class
// ============================================================================

#include <gtest/gtest.h>
#include "OrderQueue.h"
#include <thread>
#include <chrono>

using namespace orderbook;

// ============================================================================
// BASIC QUEUE OPERATIONS
// ============================================================================

TEST(OrderQueueTest, Push_SingleOrder_IncreasesSize) {
    OrderQueue queue;
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    
    queue.push(order);
    
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);
}

TEST(OrderQueueTest, TryPop_EmptyQueue_ReturnsEmpty) {
    OrderQueue queue;
    
    auto result = queue.tryPop();
    
    EXPECT_FALSE(result.has_value());
}

TEST(OrderQueueTest, TryPop_NonEmptyQueue_ReturnsOrder) {
    OrderQueue queue;
    Order order(42, Side::SELL, OrderType::LIMIT, 105.0, 200);
    queue.push(order);
    
    auto result = queue.tryPop();
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->getId(), 42);
    EXPECT_EQ(result->getSide(), Side::SELL);
}

TEST(OrderQueueTest, FIFO_OrdersReturnedInCorrectOrder) {
    OrderQueue queue;
    queue.push(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    queue.push(Order(2, Side::BUY, OrderType::LIMIT, 101.0, 100));
    queue.push(Order(3, Side::BUY, OrderType::LIMIT, 102.0, 100));
    
    auto first = queue.tryPop();
    auto second = queue.tryPop();
    auto third = queue.tryPop();
    
    EXPECT_EQ(first->getId(), 1);
    EXPECT_EQ(second->getId(), 2);
    EXPECT_EQ(third->getId(), 3);
}

// ============================================================================
// CLEAR AND SIZE TESTS
// ============================================================================

TEST(OrderQueueTest, Clear_RemovesAllOrders) {
    OrderQueue queue;
    queue.push(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    queue.push(Order(2, Side::BUY, OrderType::LIMIT, 101.0, 100));
    
    queue.clear();
    
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST(OrderQueueTest, Size_ReflectsQueueContents) {
    OrderQueue queue;
    
    EXPECT_EQ(queue.size(), 0);
    
    queue.push(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    EXPECT_EQ(queue.size(), 1);
    
    queue.push(Order(2, Side::BUY, OrderType::LIMIT, 101.0, 100));
    EXPECT_EQ(queue.size(), 2);
    
    queue.tryPop();
    EXPECT_EQ(queue.size(), 1);
}

// ============================================================================
// TIMEOUT TESTS
// ============================================================================

TEST(OrderQueueTest, PopWithTimeout_EmptyQueue_ReturnsAfterTimeout) {
    OrderQueue queue;
    
    auto start = std::chrono::steady_clock::now();
    auto result = queue.popWithTimeout(100);  // 100ms timeout
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_FALSE(result.has_value());
    EXPECT_GE(elapsed.count(), 90);  // Should wait at least ~100ms
}

TEST(OrderQueueTest, PopWithTimeout_OrderArrives_ReturnsImmediately) {
    OrderQueue queue;
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    queue.push(order);
    
    auto start = std::chrono::steady_clock::now();
    auto result = queue.popWithTimeout(1000);  // 1s timeout
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_LT(elapsed.count(), 100);  // Should return quickly
}

// ============================================================================
// SHUTDOWN TESTS
// ============================================================================

TEST(OrderQueueTest, Shutdown_StopsBlockingPop) {
    OrderQueue queue;
    
    std::atomic<bool> popReturned{false};
    std::optional<Order> result;
    
    // Start a thread that will block on pop
    std::thread consumer([&]() {
        result = queue.pop();
        popReturned = true;
    });
    
    // Give the thread time to start blocking
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Signal shutdown
    queue.shutdown();
    
    // Wait for thread to finish
    consumer.join();
    
    EXPECT_TRUE(popReturned);
    EXPECT_FALSE(result.has_value());  // Should return empty due to shutdown
}

TEST(OrderQueueTest, IsShutdown_ReturnsTrueAfterShutdown) {
    OrderQueue queue;
    
    EXPECT_FALSE(queue.isShutdown());
    
    queue.shutdown();
    
    EXPECT_TRUE(queue.isShutdown());
}

// ============================================================================
// THREAD SAFETY TESTS
// ============================================================================

TEST(OrderQueueTest, ThreadSafety_MultipleProducers) {
    OrderQueue queue;
    const int ordersPerThread = 100;
    const int numThreads = 4;
    
    std::vector<std::thread> producers;
    
    for (int t = 0; t < numThreads; ++t) {
        producers.emplace_back([&queue, t, ordersPerThread]() {
            for (int i = 0; i < ordersPerThread; ++i) {
                OrderId id = t * 1000 + i;
                queue.push(Order(id, Side::BUY, OrderType::LIMIT, 100.0, 100));
            }
        });
    }
    
    for (auto& thread : producers) {
        thread.join();
    }
    
    EXPECT_EQ(queue.size(), numThreads * ordersPerThread);
}

TEST(OrderQueueTest, ThreadSafety_ProducerConsumer) {
    OrderQueue queue;
    const int totalOrders = 1000;
    std::atomic<int> consumed{0};
    
    // Producer thread
    std::thread producer([&queue, totalOrders]() {
        for (int i = 0; i < totalOrders; ++i) {
            queue.push(Order(i, Side::BUY, OrderType::LIMIT, 100.0, 100));
        }
    });
    
    // Consumer thread
    std::thread consumer([&queue, &consumed, totalOrders]() {
        while (consumed < totalOrders) {
            auto order = queue.popWithTimeout(100);
            if (order.has_value()) {
                consumed++;
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(consumed, totalOrders);
}
