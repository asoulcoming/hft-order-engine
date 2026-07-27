#include "hft/book/order_book.hpp"
#include <sstream>
#include <algorithm>

namespace hft {

// ── 订单存储 ─────────────────────────────────────────────
Order* OrderBook::store_order(Order order) {
    auto [it, _] = orders_.insert_or_assign(order.id, order);
    return &it->second;  // map 的引用稳定，返回指针指向这里
}

void OrderBook::erase_order(OrderId id) {
    orders_.erase(id);
}

// ── FOK 可行性检查 ─────────────────────────────────────
// 遍历对面订单簿，累加可用量，判断能否完全吃下
bool OrderBook::can_fill_fok(const Order& order) const {
    Quantity needed = order.quantity;

    if (order.side == Side::Buy) {
        for (const auto& [price, level] : asks_) {
            if (order.price < price) break;  // 价格不交叉，停止
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
    return false;  // 吃不完，FOK 失败
}

// ── 主动撮合 ────────────────────────────────────────────
// taker 订单逐步吃掉对面最优价位的流动性
void OrderBook::match_aggressive(Order& taker, std::vector<Event>& out) {
    if (taker.side == Side::Buy) {
        // 买方主动：遍历 asks（最低卖价开始）
        while (taker.remaining() > 0 && !asks_.empty()) {
            auto it = asks_.begin();
            const Price best_ask = it->first;
            PriceLevel& level = it->second;

            // 限价单：买方出价低于最低卖价 → 无法成交，停止
            // 市价单：入参时已设 price=INT64_MAX，永远不会走到这里
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
                level.pop_front();       // 从队列移除
                erase_order(maker->id);  // 从存储删除
                if (level.empty()) {
                    asks_.erase(it);     // 价位清空 → 删除整个档位
                }
            }
        }
    } else {
        // 卖方主动：遍历 bids（最高买价开始），逻辑对称
        while (taker.remaining() > 0 && !bids_.empty()) {
            auto it = bids_.begin();
            const Price best_bid = it->first;
            PriceLevel& level = it->second;

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

// ── 提交限价单（主入口）─────────────────────────────────
void OrderBook::submit(Order order, std::vector<Event>& out) {
    // 1. 查重
    if (orders_.contains(order.id)) {
        out.push_back(RejectedEvent{order.id, RejectReason::DuplicateOrderId});
        return;
    }

    // 2. FOK 预检查：流动性不够就直接拒绝，不撮也不挂
    if (order.type == OrderType::FOK) {
        if (!can_fill_fok(order)) {
            out.push_back(RejectedEvent{order.id, RejectReason::InsufficientLiquidity});
            return;
        }
    }

    // 3. 撮合
    match_aggressive(order, out);

    // 4. GFD 且有余量 → 挂入本方订单簿
    if (order.type == OrderType::GFD && order.remaining() > 0) {
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
    // IOC/Market 有余量：静默丢弃（不挂单）
}

// ── 撤单 ─────────────────────────────────────────────────
void OrderBook::cancel(OrderId order_id, std::vector<Event>& out) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        out.push_back(RejectedEvent{order_id, RejectReason::UnknownOrderId});
        return;
    }

    const Order& order = it->second;
    Price price = order.price;
    Side side = order.side;

    // 从对应价位的队列中移除
    if (side == Side::Buy) {
        auto level_it = bids_.find(price);
        level_it->second.remove(order_id);
        if (level_it->second.empty()) {
            bids_.erase(level_it);  // 价位空了就删除整个档位
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

// ── 改单（等价于内部撤单 + 重新提交）────────────────────
void OrderBook::modify(OrderId order_id, Price new_price, Quantity new_qty,
                       std::vector<Event>& out) {
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        out.push_back(RejectedEvent{order_id, RejectReason::UnknownOrderId});
        return;
    }

    // 复制原订单，更新价格与数量
    Order updated = it->second;
    updated.price = new_price;
    updated.quantity = new_qty;
    updated.filled_qty = 0;

    // 内部撤单（不产生事件）
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

    // 重新提交（会产生 Accept/Reject/Trade 事件）
    submit(updated, out);
}

// ── 市价单 ───────────────────────────────────────────────
void OrderBook::market(Order order, std::vector<Event>& out) {
    // 市价单：设极端价格以吃掉所有对面流动性，行为类似 IOC
    order.price = (order.side == Side::Buy) ? INT64_MAX : 0;
    order.type = OrderType::IOC;
    submit(order, out);
}

// ── 订单簿快照 ───────────────────────────────────────────
void OrderBook::print(std::vector<Event>& out) const {
    std::ostringstream oss;
    oss << "=== Order Book ===\n";

    if (!asks_.empty()) {
        oss << "Asks (Sell):\n";
        for (const auto& [price, level] : asks_) {
            oss << "  " << price << " : " << level.total_volume;
            for (const auto* o : level.orders) {
                oss << "  [id=" << o->id << " qty=" << o->remaining() << "]";
            }
            oss << "\n";
        }
    }

    if (!bids_.empty()) {
        oss << "Bids (Buy):\n";
        for (const auto& [price, level] : bids_) {
            oss << "  " << price << " : " << level.total_volume;
            for (const auto* o : level.orders) {
                oss << "  [id=" << o->id << " qty=" << o->remaining() << "]";
            }
            oss << "\n";
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

// ── 调试快照（供测试验证订单簿结构）────────────────────
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
