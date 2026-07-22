#include "hft/book/price_level.hpp"
#include "hft/core/order.hpp"
#include <gtest/gtest.h>

using namespace hft;

class PriceLevelTest : public ::testing::Test {
protected:
    Order buy1_{1, Side::Buy, OrderType::GTC, 10000, 100};
    Order buy2_{2, Side::Buy, OrderType::GTC, 10000, 50};
    Order buy3_{3, Side::Buy, OrderType::GTC, 10000, 200};
};

TEST_F(PriceLevelTest, PushBackIncreasesCountAndVolume) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    EXPECT_EQ(level.total_volume, 100);
    EXPECT_FALSE(level.empty());
}

TEST_F(PriceLevelTest, PushBackMultipleOrdersFIFO) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    level.push_back(&buy2_);
    EXPECT_EQ(level.total_volume, 150);
    EXPECT_EQ(level.front()->id, 1);
}

TEST_F(PriceLevelTest, PopFrontRemovesOldestAndUpdatesVolume) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    level.push_back(&buy2_);
    level.pop_front();
    EXPECT_EQ(level.front()->id, 2);
    EXPECT_EQ(level.total_volume, 50);
}

TEST_F(PriceLevelTest, PopFrontOnEmptyIsSafe) {
    PriceLevel level{10000};
    EXPECT_NO_THROW(level.pop_front());
    EXPECT_TRUE(level.empty());
}

TEST_F(PriceLevelTest, RemoveMiddleOrder) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    level.push_back(&buy2_);
    level.push_back(&buy3_);
    EXPECT_TRUE(level.remove(2));
    EXPECT_EQ(level.total_volume, 300);
    EXPECT_EQ(level.front()->id, 1);
    level.pop_front();
    EXPECT_EQ(level.front()->id, 3);
}

TEST_F(PriceLevelTest, RemoveLastOrderEmptiesLevel) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    EXPECT_TRUE(level.remove(1));
    EXPECT_TRUE(level.empty());
    EXPECT_EQ(level.total_volume, 0);
}

TEST_F(PriceLevelTest, RemoveNonExistentReturnsFalse) {
    PriceLevel level{10000};
    level.push_back(&buy1_);
    EXPECT_FALSE(level.remove(999));
    EXPECT_EQ(level.total_volume, 100);
}

TEST_F(PriceLevelTest, EmptyLevelFrontReturnsNullptr) {
    PriceLevel level{10000};
    EXPECT_EQ(level.front(), nullptr);
}
