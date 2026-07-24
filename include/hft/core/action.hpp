#pragma once

#include "hft/core/types.hpp"
#include <string>
#include <variant>

namespace hft {

// 下单命令 — 提交一个新的限价单
struct SubmitAction {
    OrderId   id;               // 订单号
    Side      side;             // 买卖方向
    OrderType type = OrderType::GTC;  // 订单类型
    Price     price;            // 限价
    Quantity  quantity;         // 数量
};

// 撤单命令
struct CancelAction   { OrderId order_id; };
// 改单命令
struct ModifyAction   { OrderId order_id; Price new_price; Quantity new_qty; };
// 市价单命令
struct MarketAction   { OrderId id; Side side; Quantity qty; };
// 打印订单簿命令
struct PrintAction    {};

// 所有命令的合集
using Action = std::variant<SubmitAction, CancelAction, ModifyAction,
                            MarketAction, PrintAction>;

} // namespace hft
