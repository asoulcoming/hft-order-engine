#include "hft/book/order_book.hpp"
#include <sstream>
#include <algorithm>

namespace hft {

Order* OrderBook::store_order(Order order) {
    auto [it, _] = orders_.insert_or_assign(order.id, order);
    return &it->second;
}

void OrderBook::erase_order(OrderId id) {
    orders_.erase(id);
}

bool OrderBook::can_fill_fok(const Order& order) const {
    Quantity needed = order.quantity;

    if (order.side == Side::Buy) {
        for (const auto& [price, level] : asks_) {
            if (order.price < price) break;
            Quantity take = std::min(needed, level.total_volume);
            needed -= take;
            if (needed == 0) return true;
        }
    } else {
        for (const auto& [price, level] : bids_) {
            if (order.price > price) break;
            Quantity take = std::min(needed, level.total_volume);
            needed -= take;
            if (needed == 0) return true;
        }
    }
    return false;
}

void OrderBook::match_aggressive(Order& taker, std::vector<Event>& out) {
    if (taker.side == Side::Buy) {
        while (taker.remaining() > 0 && !asks_.empty()) {
            auto it = asks_.begin();
            const Price best_ask = it->first;
            PriceLevel& level = it->second;

            // GTC/IOC/FOK: stop if price doesn't cross.
            // Market orders enter with price=INT64_MAX so never break here.
            if (taker.price < best_ask) break;

            Order* maker = level.front();
            Quantity fill_qty = std::min(taker.remaining(), maker->remaining());

            out.push_back(TradeEvent{
                maker->id, taker.id, maker->price, fill_qty
            });

            taker.filled_qty += fill_qty;
            maker->filled_qty += fill_qty;
            level.total_volume -= fill_qty;

            if (maker->is_filled()) {
                level.pop_front();
                erase_order(maker->id);
                if (level.empty()) {
                    asks_.erase(it);
                }
            }
        }
    } else {
        while (taker.remaining() > 0 && !bids_.empty()) {
            auto it = bids_.begin();
            const Price best_bid = it->first;
            PriceLevel& level = it->second;

            // GTC/IOC/FOK: stop if price doesn't cross.
            // Market orders enter with price=0 so never break here.
            if (taker.price > best_bid) break;

            Order* maker = level.front();
            Quantity fill_qty = std::min(taker.remaining(), maker->remaining());

            out.push_back(TradeEvent{
                maker->id, taker.id, maker->price, fill_qty
            });

            taker.filled_qty += fill_qty;
            maker->filled_qty += fill_qty;
            level.total_volume -= fill_qty;

            if (maker->is_filled()) {
                level.pop_front();
                erase_order(maker->id);
                if (level.empty()) {
                    bids_.erase(it);
                }
            }
        }
    }
}

void OrderBook::submit(Order order, std::vector<Event>& out) {
    if (orders_.contains(order.id)) {
        out.push_back(RejectedEvent{order.id, RejectReason::DuplicateOrderId});
        return;
    }

    if (order.type == OrderType::FOK) {
        if (!can_fill_fok(order)) {
            out.push_back(RejectedEvent{order.id, RejectReason::InsufficientLiquidity});
            return;
        }
    }

    match_aggressive(order, out);

    if (order.type == OrderType::GTC && order.remaining() > 0) {
        Order* stored = store_order(order);

        if (order.side == Side::Buy) {
            auto it = bids_.find(order.price);
            if (it == bids_.end()) {
                it = bids_.emplace(order.price, PriceLevel{order.price}).first;
            }
            it->second.push_back(stored);
        } else {
            auto it = asks_.find(order.price);
            if (it == asks_.end()) {
                it = asks_.emplace(order.price, PriceLevel{order.price}).first;
            }
            it->second.push_back(stored);
        }

        out.push_back(AcceptedEvent{order.id});
    }
}

void OrderBook::cancel(OrderId order_id, std::vector<Event>& out) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        out.push_back(RejectedEvent{order_id, RejectReason::UnknownOrderId});
        return;
    }

    const Order& order = it->second;
    Price price = order.price;
    Side side = order.side;

    if (side == Side::Buy) {
        auto level_it = bids_.find(price);
        level_it->second.remove(order_id);
        if (level_it->second.empty()) {
            bids_.erase(level_it);
        }
    } else {
        auto level_it = asks_.find(price);
        level_it->second.remove(order_id);
        if (level_it->second.empty()) {
            asks_.erase(level_it);
        }
    }

    erase_order(order_id);
    out.push_back(CanceledEvent{order_id});
}

void OrderBook::modify(OrderId order_id, Price new_price, Quantity new_qty,
                       std::vector<Event>& out) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        out.push_back(RejectedEvent{order_id, RejectReason::UnknownOrderId});
        return;
    }

    Order updated = it->second;
    updated.price = new_price;
    updated.quantity = new_qty;
    updated.filled_qty = 0;

    // Internal cancel (no events emitted)
    {
        Price old_price = it->second.price;
        Side side = it->second.side;

        if (side == Side::Buy) {
            auto level_it = bids_.find(old_price);
            level_it->second.remove(order_id);
            if (level_it->second.empty()) {
                bids_.erase(level_it);
            }
        } else {
            auto level_it = asks_.find(old_price);
            level_it->second.remove(order_id);
            if (level_it->second.empty()) {
                asks_.erase(level_it);
            }
        }
        erase_order(order_id);
    }

    submit(updated, out);
}

void OrderBook::market(Order order, std::vector<Event>& out) {
    order.price = (order.side == Side::Buy) ? INT64_MAX : 0;
    order.type = OrderType::IOC;
    submit(order, out);
}

void OrderBook::print(std::vector<Event>& out) const {
    std::ostringstream oss;
    oss << "=== Order Book ===\n";

    if (!asks_.empty()) {
        oss << "Asks (Sell):\n";
        for (const auto& [price, level] : asks_) {
            oss << "  " << price << " : " << level.total_volume << "\n";
        }
    }

    if (!bids_.empty()) {
        oss << "Bids (Buy):\n";
        for (const auto& [price, level] : bids_) {
            oss << "  " << price << " : " << level.total_volume << "\n";
        }
    }

    if (bids_.empty() && asks_.empty()) {
        oss << "(empty)\n";
    }

    out.push_back(SnapshotEvent{oss.str()});
}

bool OrderBook::contains(OrderId id) const {
    return orders_.contains(id);
}

OrderBook::DebugSnapshot OrderBook::debug_snapshot() const {
    DebugSnapshot snap;

    for (const auto& [price, level] : bids_) {
        DebugLevel dl;
        dl.price = price;
        dl.volume = level.total_volume;
        for (const auto* o : level.orders) {
            dl.order_ids.push_back(o->id);
        }
        snap.bids.push_back(std::move(dl));
    }

    for (const auto& [price, level] : asks_) {
        DebugLevel dl;
        dl.price = price;
        dl.volume = level.total_volume;
        for (const auto* o : level.orders) {
            dl.order_ids.push_back(o->id);
        }
        snap.asks.push_back(std::move(dl));
    }

    return snap;
}

} // namespace hft
