#pragma once

#include "hft/core/types.hpp"
#include <string>
#include <variant>

namespace hft {

struct SubmitAction {
    OrderId   id;
    Side      side;
    OrderType type = OrderType::GTC;
    Price     price;
    Quantity  quantity;
};

struct CancelAction   { OrderId order_id; };
struct ModifyAction   { OrderId order_id; Price new_price; Quantity new_qty; };
struct MarketAction   { OrderId id; Side side; Quantity qty; };
struct PrintAction    {};

using Action = std::variant<SubmitAction, CancelAction, ModifyAction,
                            MarketAction, PrintAction>;

} // namespace hft
