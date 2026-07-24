#include "hft/io/formatter.hpp"
#include <sstream>
#include <string>

namespace hft {

// 将每种 Event 类型转换为单行可读文本
std::string format_event(const Event& event) {
    return std::visit([](const auto& e) -> std::string {
        using T = std::decay_t<decltype(e)>;

        // 成交事件
        if constexpr (std::is_same_v<T, TradeEvent>) {
            std::ostringstream oss;
            oss << "TRADE resting=" << e.resting_id
                << " incoming=" << e.aggressive_id
                << " price=" << e.price
                << " qty=" << e.qty;
            return oss.str();
        }

        // 订单已接受
        if constexpr (std::is_same_v<T, AcceptedEvent>) {
            return "ACCEPTED " + std::to_string(e.order_id);
        }

        // 订单已撤销
        if constexpr (std::is_same_v<T, CanceledEvent>) {
            return "CANCELED " + std::to_string(e.order_id);
        }

        // 订单被拒绝
        if constexpr (std::is_same_v<T, RejectedEvent>) {
            std::string reason;
            switch (e.reason) {
                case RejectReason::DuplicateOrderId:
                    reason = "duplicate order id"; break;
                case RejectReason::UnknownOrderId:
                    reason = "unknown order id"; break;
                case RejectReason::InsufficientLiquidity:
                    reason = "insufficient liquidity"; break;
                case RejectReason::InvalidPrice:
                    reason = "invalid price"; break;
                case RejectReason::InvalidQuantity:
                    reason = "invalid quantity"; break;
                default:
                    reason = "unknown"; break;
            }
            return "REJECTED " + reason + " " + std::to_string(e.order_id);
        }

        // 订单簿快照（message 已由 OrderBook::print 格式化好）
        if constexpr (std::is_same_v<T, SnapshotEvent>) {
            return e.message;
        }

        return "UNKNOWN_EVENT";
    }, event);
}

} // namespace hft
