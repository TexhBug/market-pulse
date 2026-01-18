// ============================================================================
// TEST_ORDERBOOK.CPP - Unit tests for OrderBook class
// ============================================================================

#include <gtest/gtest.h>
#include "OrderBook.h"

using namespace orderbook;

// ============================================================================
// ADD ORDER TESTS
// ============================================================================

TEST(OrderBookTest, AddOrder_SingleBid_SucceedsAndVisible) {
    OrderBook book;
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    
    EXPECT_TRUE(book.addOrder(order));
    
    auto bestBid = book.getBestBid();
    ASSERT_TRUE(bestBid.has_value());
    EXPECT_DOUBLE_EQ(*bestBid, 100.0);
}

TEST(OrderBookTest, AddOrder_SingleAsk_SucceedsAndVisible) {
    OrderBook book;
    Order order(1, Side::SELL, OrderType::LIMIT, 105.0, 100);
    
    EXPECT_TRUE(book.addOrder(order));
    
    auto bestAsk = book.getBestAsk();
    ASSERT_TRUE(bestAsk.has_value());
    EXPECT_DOUBLE_EQ(*bestAsk, 105.0);
}

TEST(OrderBookTest, AddOrder_DuplicateId_Fails) {
    OrderBook book;
    Order order1(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    Order order2(1, Side::SELL, OrderType::LIMIT, 105.0, 50);
    
    EXPECT_TRUE(book.addOrder(order1));
    EXPECT_FALSE(book.addOrder(order2));
}

TEST(OrderBookTest, AddOrder_MultipleBids_SortedDescending) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 99.0, 100));
    book.addOrder(Order(2, Side::BUY, OrderType::LIMIT, 101.0, 100));
    book.addOrder(Order(3, Side::BUY, OrderType::LIMIT, 100.0, 100));
    
    auto bestBid = book.getBestBid();
    ASSERT_TRUE(bestBid.has_value());
    EXPECT_DOUBLE_EQ(*bestBid, 101.0);  // Highest bid first
}

TEST(OrderBookTest, AddOrder_MultipleAsks_SortedAscending) {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, OrderType::LIMIT, 103.0, 100));
    book.addOrder(Order(2, Side::SELL, OrderType::LIMIT, 101.0, 100));
    book.addOrder(Order(3, Side::SELL, OrderType::LIMIT, 102.0, 100));
    
    auto bestAsk = book.getBestAsk();
    ASSERT_TRUE(bestAsk.has_value());
    EXPECT_DOUBLE_EQ(*bestAsk, 101.0);  // Lowest ask first
}

// ============================================================================
// CANCEL ORDER TESTS
// ============================================================================

TEST(OrderBookTest, CancelOrder_ExistingOrder_Succeeds) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    
    EXPECT_TRUE(book.cancelOrder(1));
    EXPECT_FALSE(book.getBestBid().has_value());  // No more bids
}

TEST(OrderBookTest, CancelOrder_NonExistingOrder_Fails) {
    OrderBook book;
    
    EXPECT_FALSE(book.cancelOrder(999));
}

TEST(OrderBookTest, CancelOrder_RemovesPriceLevelIfEmpty) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    book.addOrder(Order(2, Side::BUY, OrderType::LIMIT, 99.0, 100));
    
    book.cancelOrder(1);
    
    auto bestBid = book.getBestBid();
    ASSERT_TRUE(bestBid.has_value());
    EXPECT_DOUBLE_EQ(*bestBid, 99.0);  // Now 99.0 is best
}

// ============================================================================
// SPREAD TESTS
// ============================================================================

TEST(OrderBookTest, GetSpread_ValidBidAndAsk_ReturnsCorrectSpread) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    book.addOrder(Order(2, Side::SELL, OrderType::LIMIT, 101.5, 100));
    
    auto spread = book.getSpread();
    ASSERT_TRUE(spread.has_value());
    EXPECT_DOUBLE_EQ(*spread, 1.5);
}

TEST(OrderBookTest, GetSpread_NoBids_ReturnsEmpty) {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, OrderType::LIMIT, 101.0, 100));
    
    auto spread = book.getSpread();
    EXPECT_FALSE(spread.has_value());
}

TEST(OrderBookTest, GetSpread_NoAsks_ReturnsEmpty) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    
    auto spread = book.getSpread();
    EXPECT_FALSE(spread.has_value());
}

// ============================================================================
// QUANTITY AT PRICE TESTS
// ============================================================================

TEST(OrderBookTest, GetQuantityAtPrice_SingleOrder_ReturnsQuantity) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 150));
    
    EXPECT_EQ(book.getQuantityAtPrice(Side::BUY, 100.0), 150);
}

TEST(OrderBookTest, GetQuantityAtPrice_MultipleOrdersSamePrice_ReturnsTotalQuantity) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    book.addOrder(Order(2, Side::BUY, OrderType::LIMIT, 100.0, 200));
    book.addOrder(Order(3, Side::BUY, OrderType::LIMIT, 100.0, 50));
    
    EXPECT_EQ(book.getQuantityAtPrice(Side::BUY, 100.0), 350);
}

TEST(OrderBookTest, GetQuantityAtPrice_NonExistingPrice_ReturnsZero) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    
    EXPECT_EQ(book.getQuantityAtPrice(Side::BUY, 99.0), 0);
}

// ============================================================================
// TOP BIDS/ASKS TESTS
// ============================================================================

TEST(OrderBookTest, GetTopBids_ReturnsCorrectLevels) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    book.addOrder(Order(2, Side::BUY, OrderType::LIMIT, 99.0, 200));
    book.addOrder(Order(3, Side::BUY, OrderType::LIMIT, 98.0, 300));
    
    auto bids = book.getTopBids(2);
    
    ASSERT_EQ(bids.size(), 2);
    EXPECT_DOUBLE_EQ(bids[0].first, 100.0);  // Highest first
    EXPECT_EQ(bids[0].second, 100);
    EXPECT_DOUBLE_EQ(bids[1].first, 99.0);
    EXPECT_EQ(bids[1].second, 200);
}

TEST(OrderBookTest, GetTopAsks_ReturnsCorrectLevels) {
    OrderBook book;
    book.addOrder(Order(1, Side::SELL, OrderType::LIMIT, 102.0, 100));
    book.addOrder(Order(2, Side::SELL, OrderType::LIMIT, 101.0, 200));
    book.addOrder(Order(3, Side::SELL, OrderType::LIMIT, 103.0, 300));
    
    auto asks = book.getTopAsks(2);
    
    ASSERT_EQ(asks.size(), 2);
    EXPECT_DOUBLE_EQ(asks[0].first, 101.0);  // Lowest first
    EXPECT_EQ(asks[0].second, 200);
    EXPECT_DOUBLE_EQ(asks[1].first, 102.0);
    EXPECT_EQ(asks[1].second, 100);
}

// ============================================================================
// STATISTICS TESTS
// ============================================================================

TEST(OrderBookTest, GetTotalOrderCount_ReturnsCorrectCount) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    book.addOrder(Order(2, Side::SELL, OrderType::LIMIT, 101.0, 100));
    book.addOrder(Order(3, Side::BUY, OrderType::LIMIT, 99.0, 100));
    
    EXPECT_EQ(book.getTotalOrderCount(), 3);
}

TEST(OrderBookTest, GetBidLevelCount_ReturnsCorrectCount) {
    OrderBook book;
    book.addOrder(Order(1, Side::BUY, OrderType::LIMIT, 100.0, 100));
    book.addOrder(Order(2, Side::BUY, OrderType::LIMIT, 100.0, 50));  // Same level
    book.addOrder(Order(3, Side::BUY, OrderType::LIMIT, 99.0, 100));  // Different level
    
    EXPECT_EQ(book.getBidLevelCount(), 2);  // 2 price levels
}
