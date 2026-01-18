// ============================================================================
// TEST_ORDER.CPP - Unit tests for Order class
// ============================================================================

#include <gtest/gtest.h>
#include "Order.h"

using namespace orderbook;

// ============================================================================
// CONSTRUCTOR TESTS
// ============================================================================

TEST(OrderTest, Constructor_CreatesOrderWithCorrectValues) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.50, 200);
    
    EXPECT_EQ(order.getId(), 1);
    EXPECT_EQ(order.getSide(), Side::BUY);
    EXPECT_EQ(order.getType(), OrderType::LIMIT);
    EXPECT_DOUBLE_EQ(order.getPrice(), 100.50);
    EXPECT_EQ(order.getQuantity(), 200);
    EXPECT_EQ(order.getFilledQty(), 0);
    EXPECT_EQ(order.getRemainingQty(), 200);
    EXPECT_EQ(order.getStatus(), OrderStatus::NEW);
}

TEST(OrderTest, DefaultConstructor_CreatesEmptyOrder) {
    Order order;
    
    EXPECT_EQ(order.getId(), 0);
    EXPECT_EQ(order.getQuantity(), 0);
}

// ============================================================================
// FILL TESTS
// ============================================================================

TEST(OrderTest, Fill_PartialFill_UpdatesQuantitiesAndStatus) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    
    EXPECT_TRUE(order.fill(30));
    
    EXPECT_EQ(order.getFilledQty(), 30);
    EXPECT_EQ(order.getRemainingQty(), 70);
    EXPECT_EQ(order.getStatus(), OrderStatus::PARTIAL);
}

TEST(OrderTest, Fill_CompleteFill_SetsStatusToFilled) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    
    EXPECT_TRUE(order.fill(100));
    
    EXPECT_EQ(order.getFilledQty(), 100);
    EXPECT_EQ(order.getRemainingQty(), 0);
    EXPECT_EQ(order.getStatus(), OrderStatus::FILLED);
}

TEST(OrderTest, Fill_MultipleFills_AccumulatesCorrectly) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    
    EXPECT_TRUE(order.fill(30));
    EXPECT_TRUE(order.fill(40));
    EXPECT_TRUE(order.fill(30));
    
    EXPECT_EQ(order.getFilledQty(), 100);
    EXPECT_EQ(order.getStatus(), OrderStatus::FILLED);
}

TEST(OrderTest, Fill_OverFill_ReturnsFalse) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    
    EXPECT_FALSE(order.fill(150));
    
    // Order should be unchanged
    EXPECT_EQ(order.getFilledQty(), 0);
    EXPECT_EQ(order.getStatus(), OrderStatus::NEW);
}

// ============================================================================
// CANCEL TESTS
// ============================================================================

TEST(OrderTest, Cancel_ActiveOrder_SetsCancelledStatus) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    
    order.cancel();
    
    EXPECT_EQ(order.getStatus(), OrderStatus::CANCELLED);
    EXPECT_FALSE(order.isActive());
}

TEST(OrderTest, Cancel_FilledOrder_DoesNotChange) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    order.fill(100);  // Fully fill
    
    order.cancel();
    
    EXPECT_EQ(order.getStatus(), OrderStatus::FILLED);
}

// ============================================================================
// MODIFY TESTS
// ============================================================================

TEST(OrderTest, ModifyPrice_UnfilledLimitOrder_Succeeds) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    
    EXPECT_TRUE(order.modifyPrice(105.0));
    EXPECT_DOUBLE_EQ(order.getPrice(), 105.0);
}

TEST(OrderTest, ModifyPrice_PartiallyFilledOrder_Fails) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    order.fill(50);
    
    EXPECT_FALSE(order.modifyPrice(105.0));
    EXPECT_DOUBLE_EQ(order.getPrice(), 100.0);
}

TEST(OrderTest, ModifyQuantity_IncreaseAllowed) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    
    EXPECT_TRUE(order.modifyQuantity(150));
    EXPECT_EQ(order.getQuantity(), 150);
}

TEST(OrderTest, ModifyQuantity_DecreaseBelowFilled_Fails) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    order.fill(60);
    
    EXPECT_FALSE(order.modifyQuantity(50));
    EXPECT_EQ(order.getQuantity(), 100);
}

// ============================================================================
// UTILITY TESTS
// ============================================================================

TEST(OrderTest, IsActive_NewOrder_ReturnsTrue) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    EXPECT_TRUE(order.isActive());
}

TEST(OrderTest, IsActive_CancelledOrder_ReturnsFalse) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    order.cancel();
    EXPECT_FALSE(order.isActive());
}

TEST(OrderTest, IsActive_FilledOrder_ReturnsFalse) {
    Order order(1, Side::BUY, OrderType::LIMIT, 100.0, 100);
    order.fill(100);
    EXPECT_FALSE(order.isActive());
}

TEST(OrderTest, ToString_ReturnsReadableString) {
    Order order(42, Side::SELL, OrderType::LIMIT, 150.75, 500);
    
    std::string str = order.toString();
    
    EXPECT_NE(str.find("42"), std::string::npos);
    EXPECT_NE(str.find("SELL"), std::string::npos);
    EXPECT_NE(str.find("500"), std::string::npos);
    EXPECT_NE(str.find("150.75"), std::string::npos);
}
