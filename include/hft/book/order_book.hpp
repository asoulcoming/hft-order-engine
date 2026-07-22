#pragma once

#include "hft/book/price_level.hpp"
#include "hft/core/event.hpp"
#include "hft/core/order.hpp"
#include <map>
#include <unordered_map>
#include <vector>

namespace hft {

class OrderBook {
public:
    void submit(Order order, std::vector<Event>& out);
    void cancel(OrderId order_id, std::vector<Event>& out);
    void modify(OrderId order_id, Price new_price, Quantity new_qty,
                std::vector<Event>& out);
    void market(Order order, std::vector<Event>& out);
    void print(std::vector<Event>& out) const;

    [[nodiscard]] bool contains(OrderId id) const;

    struct DebugLevel {
        Price                 price;
        Quantity              volume;
        std::vector<OrderId>  order_ids;
    };
    struct DebugSnapshot {
        std::vector<DebugLevel> bids;
        std::vector<DebugLevel> asks;
    };
    [[nodiscard]] DebugSnapshot debug_snapshot() const;

private:
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::map<Price, PriceLevel> asks_;
    std::unordered_map<OrderId, Order> orders_;

    void match_aggressive(Order& taker, std::vector<Event>& out);
    [[nodiscard]] bool can_fill_fok(const Order& order) const;
    Order* store_order(Order order);
    void   erase_order(OrderId id);
};

} // namespace hft
