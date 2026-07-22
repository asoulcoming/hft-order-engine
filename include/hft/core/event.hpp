#pragma once

#include "hft/core/types.hpp"
#include <string>
#include <variant>

namespace hft {

struct TradeEvent {
    OrderId  resting_id;       // passive (maker) order
    OrderId  aggressive_id;    // incoming (taker) order
    Price    price;
    Quantity qty;
};

struct AcceptedEvent   { OrderId order_id; };
struct CanceledEvent   { OrderId order_id; };
struct RejectedEvent   { OrderId order_id; RejectReason reason; };
struct SnapshotEvent   { std::string message; };

using Event = std::variant<TradeEvent, AcceptedEvent, CanceledEvent,
                           RejectedEvent, SnapshotEvent>;

} // namespace hft
