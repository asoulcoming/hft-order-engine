#pragma once

#include "hft/core/types.hpp"

namespace hft {

struct Order {
    OrderId   id;
    Side      side;
    OrderType type     = OrderType::GTC;
    Price     price;           // integer cents
    Quantity  quantity;
    Quantity  filled_qty = 0;

    [[nodiscard]] Quantity remaining() const { return quantity - filled_qty; }
    [[nodiscard]] bool     is_filled()  const { return filled_qty == quantity; }
};

} // namespace hft
