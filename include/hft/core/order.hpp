#pragma once

#include "hft/core/types.hpp"

namespace hft {

// 订单结构体 — 纯数据（POD），不包含任何指针
struct Order {
    OrderId   id;               // 订单唯一标识
    Side      side;             // 买卖方向
    OrderType type     = OrderType::GTC;  // 订单类型，默认 GTC
    Price     price;            // 限价（整数分）
    Quantity  quantity;         // 原始数量
    Quantity  filled_qty = 0;  // 已成交量

    // 剩余可成交量
    [[nodiscard]] Quantity remaining() const { return quantity - filled_qty; }
    // 是否已完全成交
    [[nodiscard]] bool     is_filled()  const { return filled_qty == quantity; }
};

} // namespace hft
