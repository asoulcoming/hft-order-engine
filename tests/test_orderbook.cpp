#include "hft/book/order_book.hpp"
#include <gtest/gtest.h>

using namespace hft;

static Order buy(uint64_t id, Price price, Quantity qty) {
    return {id, Side::Buy, OrderType::GFD, price, qty};
}
static Order sell(uint64_t id, Price price, Quantity qty) {
    return {id, Side::Sell, OrderType::GFD, price, qty};
}

TEST(OrderBookTest, AddSingleOrderNoMatch) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(buy(1, 10000, 50), events);

    ASSERT_EQ(events.size(), 1);
    EXPECT_TRUE(std::holds_alternative<AcceptedEvent>(events[0]));
    EXPECT_TRUE(book.contains(1));

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.bids[0].price, 10000);
    EXPECT_EQ(snap.bids[0].volume, 50);
}

TEST(OrderBookTest, SimpleMatchSamePrice) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(sell(1, 10000, 50), events);
    events.clear();
    book.submit(buy(2, 10000, 50), events);

    const auto* trade = std::get_if<TradeEvent>(&events[0]);
    ASSERT_NE(trade, nullptr);
    EXPECT_EQ(trade->qty, 50);
    EXPECT_EQ(trade->price, 10000);
    EXPECT_EQ(trade->resting_id, 1);
    EXPECT_EQ(trade->aggressive_id, 2);

    auto snap = book.debug_snapshot();
    EXPECT_TRUE(snap.bids.empty());
    EXPECT_TRUE(snap.asks.empty());
}

TEST(OrderBookTest, NoMatchWhenSpread) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(buy(1, 9900, 50), events);
    events.clear();
    book.submit(sell(2, 10100, 50), events);

    bool has_trade = false;
    for (const auto& e : events) {
        if (std::holds_alternative<TradeEvent>(e)) has_trade = true;
    }
    EXPECT_FALSE(has_trade);

    auto snap = book.debug_snapshot();
    EXPECT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.asks.size(), 1);
}

TEST(OrderBookTest, PartialFill) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(sell(1, 10000, 50), events);
    events.clear();
    book.submit(buy(2, 10000, 100), events);

    const auto* trade = std::get_if<TradeEvent>(&events[0]);
    ASSERT_NE(trade, nullptr);
    EXPECT_EQ(trade->qty, 50);

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.bids[0].volume, 50);
    EXPECT_EQ(snap.bids[0].order_ids[0], 2);
    EXPECT_TRUE(snap.asks.empty());
}

TEST(OrderBookTest, FifoOrderingSamePrice) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(buy(1, 10000, 10), events);
    book.submit(buy(2, 10000, 10), events);
    book.submit(buy(3, 10000, 10), events);
    events.clear();

    book.submit(sell(4, 10000, 15), events);

    int trade_count = 0;
    for (const auto& e : events) {
        if (auto* t = std::get_if<TradeEvent>(&e)) {
            trade_count++;
            if (trade_count == 1) {
                EXPECT_EQ(t->resting_id, 1);
                EXPECT_EQ(t->qty, 10);
            } else {
                EXPECT_EQ(t->resting_id, 2);
                EXPECT_EQ(t->qty, 5);
            }
        }
    }
    EXPECT_EQ(trade_count, 2);

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.bids[0].volume, 15);
}

TEST(OrderBookTest, CrossPriceSweep) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(sell(1, 10000, 20), events);
    book.submit(sell(2, 10100, 30), events);
    events.clear();

    book.submit(buy(3, 10100, 40), events);

    int trade_count = 0;
    for (const auto& e : events) {
        if (std::holds_alternative<TradeEvent>(e)) trade_count++;
    }
    EXPECT_EQ(trade_count, 2);

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.asks.size(), 1);
    EXPECT_EQ(snap.asks[0].price, 10100);
    EXPECT_EQ(snap.asks[0].volume, 10);
}

TEST(OrderBookTest, CancelExistingOrder) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(buy(1, 10000, 50), events);
    events.clear();

    book.cancel(1, events);
    ASSERT_EQ(events.size(), 1);
    EXPECT_TRUE(std::holds_alternative<CanceledEvent>(events[0]));
    EXPECT_FALSE(book.contains(1));

    auto snap = book.debug_snapshot();
    EXPECT_TRUE(snap.bids.empty());
}

TEST(OrderBookTest, CancelNonExistent) {
    OrderBook book;
    std::vector<Event> events;
    book.cancel(999, events);
    ASSERT_EQ(events.size(), 1);
    auto* rej = std::get_if<RejectedEvent>(&events[0]);
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::UnknownOrderId);
}

TEST(OrderBookTest, CancelMiddleOfQueue) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(buy(1, 10000, 10), events);
    book.submit(buy(2, 10000, 10), events);
    book.submit(buy(3, 10000, 10), events);
    events.clear();

    book.cancel(2, events);
    EXPECT_FALSE(book.contains(2));

    book.submit(sell(4, 10000, 20), events);
    bool hit1 = false, hit3 = false;
    for (const auto& e : events) {
        if (auto* t = std::get_if<TradeEvent>(&e)) {
            if (t->resting_id == 1) hit1 = true;
            if (t->resting_id == 3) hit3 = true;
        }
    }
    EXPECT_TRUE(hit1);
    EXPECT_TRUE(hit3);
}

TEST(OrderBookTest, ModifyUpdatesPriceAndQuantity) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(buy(1, 10000, 50), events);
    events.clear();

    book.modify(1, 10100, 100, events);

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.bids.size(), 1);
    EXPECT_EQ(snap.bids[0].price, 10100);
    EXPECT_EQ(snap.bids[0].volume, 100);
}

TEST(OrderBookTest, ModifyNonExistent) {
    OrderBook book;
    std::vector<Event> events;
    book.modify(999, 10000, 50, events);
    auto* rej = std::get_if<RejectedEvent>(&events[0]);
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::UnknownOrderId);
}

TEST(OrderBookTest, MarketOrderFillsAgainstResting) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(sell(1, 10000, 20), events);
    book.submit(sell(2, 10001, 30), events);
    events.clear();

    book.market({3, Side::Buy, OrderType::GFD, 0, 40}, events);

    int trade_count = 0;
    Quantity total_filled = 0;
    for (const auto& e : events) {
        if (auto* t = std::get_if<TradeEvent>(&e)) {
            trade_count++;
            total_filled += t->qty;
        }
    }
    EXPECT_EQ(trade_count, 2);
    EXPECT_EQ(total_filled, 40);

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.asks.size(), 1);
    EXPECT_EQ(snap.asks[0].volume, 10);
}

TEST(OrderBookTest, MarketOrderPartialNoRest) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(sell(1, 10000, 20), events);
    events.clear();

    book.market({2, Side::Buy, OrderType::GFD, 0, 50}, events);

    const auto* trade = std::get_if<TradeEvent>(&events[0]);
    ASSERT_NE(trade, nullptr);
    EXPECT_EQ(trade->qty, 20);

    auto snap = book.debug_snapshot();
    EXPECT_TRUE(snap.bids.empty());
    EXPECT_TRUE(snap.asks.empty());
}

TEST(OrderBookTest, IocFillsPartiallyCancelsRest) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(sell(1, 10000, 20), events);
    events.clear();

    Order ioc{4, Side::Buy, OrderType::IOC, 10000, 50};
    book.submit(ioc, events);

    const auto* trade = std::get_if<TradeEvent>(&events[0]);
    ASSERT_NE(trade, nullptr);
    EXPECT_EQ(trade->qty, 20);

    auto snap = book.debug_snapshot();
    EXPECT_TRUE(snap.bids.empty());
    EXPECT_TRUE(snap.asks.empty());
}

TEST(OrderBookTest, IocWithNoMatchCancelsEntirely) {
    OrderBook book;
    std::vector<Event> events;
    Order ioc{1, Side::Buy, OrderType::IOC, 10000, 50};
    book.submit(ioc, events);

    for (const auto& e : events) {
        EXPECT_FALSE(std::holds_alternative<TradeEvent>(e));
    }
    auto snap = book.debug_snapshot();
    EXPECT_TRUE(snap.bids.empty());
}

TEST(OrderBookTest, FokFillsWhenLiquiditySufficient) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(sell(1, 10000, 50), events);
    events.clear();

    Order fok{2, Side::Buy, OrderType::FOK, 10000, 50};
    book.submit(fok, events);

    const auto* trade = std::get_if<TradeEvent>(&events[0]);
    ASSERT_NE(trade, nullptr);
    EXPECT_EQ(trade->qty, 50);
    EXPECT_FALSE(book.contains(2));
}

TEST(OrderBookTest, FokRejectsWhenInsufficient) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(sell(1, 10000, 20), events);
    events.clear();

    Order fok{2, Side::Buy, OrderType::FOK, 10000, 50};
    book.submit(fok, events);

    auto* rej = std::get_if<RejectedEvent>(&events[0]);
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::InsufficientLiquidity);

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.asks.size(), 1);
    EXPECT_EQ(snap.asks[0].volume, 20);
}

TEST(OrderBookTest, FokRejectsWhenNoLiquidity) {
    OrderBook book;
    std::vector<Event> events;
    Order fok{1, Side::Buy, OrderType::FOK, 10000, 50};
    book.submit(fok, events);

    auto* rej = std::get_if<RejectedEvent>(&events[0]);
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::InsufficientLiquidity);
}

TEST(OrderBookTest, RejectDuplicateOrderId) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(buy(1, 10000, 50), events);
    events.clear();
    book.submit(buy(1, 10001, 30), events);

    auto* rej = std::get_if<RejectedEvent>(&events[0]);
    ASSERT_NE(rej, nullptr);
    EXPECT_EQ(rej->reason, RejectReason::DuplicateOrderId);
}

TEST(OrderBookTest, DebugSnapshotCapturesBothSides) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(buy(1, 9900, 10), events);
    book.submit(buy(2, 10000, 20), events);
    book.submit(sell(3, 10100, 30), events);
    book.submit(sell(4, 10200, 40), events);

    auto snap = book.debug_snapshot();
    ASSERT_EQ(snap.bids.size(), 2);
    EXPECT_EQ(snap.bids[0].price, 10000);
    EXPECT_EQ(snap.bids[1].price, 9900);
    ASSERT_EQ(snap.asks.size(), 2);
    EXPECT_EQ(snap.asks[0].price, 10100);
    EXPECT_EQ(snap.asks[1].price, 10200);
}

TEST(OrderBookTest, PrintDoesNotCrash) {
    OrderBook book;
    std::vector<Event> events;
    book.submit(buy(1, 9900, 10), events);
    events.clear();
    book.print(events);
    ASSERT_EQ(events.size(), 1);
    EXPECT_TRUE(std::holds_alternative<SnapshotEvent>(events[0]));
}
