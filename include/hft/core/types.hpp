#pragma once

#include <cstdint>

namespace hft {

// ── Type aliases ────────────────────────────────────────────
using OrderId  = uint64_t;
using Price    = int64_t;    // integer cents (e.g. $150.00 = 15000)
using Quantity = uint64_t;

// ── Enums ───────────────────────────────────────────────────
enum class Side : uint8_t { Buy, Sell };

enum class OrderType : uint8_t {
    GTC,   // Good 'Til Cancel — rest in book if unfilled
    IOC,   // Immediate-or-Cancel — cancel unfilled remainder
    FOK,   // Fill-or-Kill — fill entirely or reject entirely
};

enum class RejectReason : uint8_t {
    DuplicateOrderId,
    UnknownOrderId,
    InsufficientLiquidity,   // FOK couldn't fill
    InvalidPrice,
    InvalidQuantity,
};

} // namespace hft
