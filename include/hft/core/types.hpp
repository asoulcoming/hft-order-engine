#pragma once

#include <cstdint>

namespace hft {

// ── 类型别名 ────────────────────────────────────────────
using OrderId  = uint64_t;   // 订单号
using Price    = int64_t;    // 价格（整数分，如 $150.00 = 15000）
using Quantity = uint64_t;   // 数量

// ── 枚举 ─────────────────────────────────────────────────
enum class Side : uint8_t { //使用uint8_t节省内存,只有一个字节，默认的类型是int，是4个字节
    Buy, 
    Sell 
};

enum class OrderType : uint8_t {
    GFD,   // 当日有效（Good For Day）— 未成交部分挂单，收盘自动撤单
    IOC,   // 立即成交否则取消 — 未成交部分丢弃
    FOK,   // 全部成交否则取消 — 不能全成则拒单
};

enum class RejectReason : uint8_t {
    DuplicateOrderId,          // 重复订单号
    UnknownOrderId,            // 未知订单号
    InsufficientLiquidity,     // 流动性不足（FOK 无法全成）
    InvalidPrice,              // 价格无效
    InvalidQuantity,           // 数量无效
};

} // namespace hft
