#pragma once

#include "hft/core/types.hpp"
#include <string>
#include <variant>

namespace hft {

// 成交事件 — 两笔订单撮合成功
struct TradeEvent {
    OrderId resting_id;    // 被动方（挂单方/maker）
    OrderId aggressive_id; // 主动方（吃单方/taker）
    Price price;           // 成交价格
    Quantity qty;          // 成交数量
};

// 订单已接受事件
struct AcceptedEvent {
    OrderId order_id;
};
// 订单已撤销事件
struct CanceledEvent {
    OrderId order_id;
};
// 订单被拒绝事件
struct RejectedEvent {
    OrderId order_id;
    RejectReason reason;
};
// 订单簿快照事件
struct SnapshotEvent {
    std::string message;
};

// 所有事件的合集
using Event = std::variant<TradeEvent, AcceptedEvent, CanceledEvent, RejectedEvent, SnapshotEvent>;

} // namespace hft
