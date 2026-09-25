#pragma once

#include "hft/book/price_level.hpp"
#include "hft/core/event.hpp"
#include "hft/core/order.hpp"
#include <map>
#include <unordered_map>
#include <vector>

namespace hft {

// 订单簿 — 撮合引擎的核心
// Iter 1 使用 std::map + std::deque（正确性优先），后续迭代逐步优化数据结构
class OrderBook {
public:
    // 提交限价单（GFD/IOC/FOK）
    void submit(Order order, std::vector<Event>& out);
    // 撤单
    void cancel(OrderId order_id, std::vector<Event>& out);
    // 改单（等价于撤单 + 重新下单）
    void modify(OrderId order_id, Price new_price, Quantity new_qty, std::vector<Event>& out);
    // 市价单（吃尽对方流动性，不挂单）
    void market(Order order, std::vector<Event>& out);
    // 打印订单簿快照
    void print(std::vector<Event>& out) const;

    // 查询指定 ID 是否仍在挂单中
    [[nodiscard]] bool contains(OrderId id) const;

    // ── 调试接口（供测试使用）──
    struct DebugLevel {
        Price price;
        Quantity volume;
        std::vector<OrderId> order_ids;
    };
    struct DebugSnapshot {
        std::vector<DebugLevel> bids;
        std::vector<DebugLevel> asks;
    };
    [[nodiscard]] DebugSnapshot debug_snapshot() const;

private:
    // 买方订单簿：降序排列，begin() = 最高买价
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    // 卖方订单簿：升序排列，begin() = 最低卖价
    std::map<Price, PriceLevel> asks_;
    // 所有挂单的存储（map 提供稳定地址，PriceLevel 持有 Order* 指向这里）
    std::unordered_map<OrderId, Order> orders_;

    // 主动撮合：taker 订单吃对面流动性
    void match_aggressive(Order& taker, std::vector<Event>& out);
    // 检查 FOK 订单是否能全部成交
    [[nodiscard]] bool can_fill_fok(const Order& order) const;
    // 存储订单并返回稳定指针
    Order* store_order(Order order);
    // 从存储中删除订单
    void erase_order(OrderId id);
};

} // namespace hft
