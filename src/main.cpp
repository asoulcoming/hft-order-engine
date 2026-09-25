#include "hft/book/order_book.hpp"
#include "hft/io/formatter.hpp"
#include "hft/io/parser.hpp"
#include <iostream>
#include <string>
#include <vector>

int main() {
    hft::OrderBook book; // 撮合引擎（单标的）
    hft::Parser parser;  // 命令解析器
    std::string line;
    std::vector<hft::Event> events; // 复用的事件缓冲区（避免反复分配）

    // 主循环：逐行读取 → 解析 → 撮合 → 输出
    while (std::getline(std::cin, line)) {
        // 处理 Windows 换行符（\r\n → \n）
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        try {
            auto action = parser.parse(line);
            if (!action)
                continue; // 空行或注释

            events.clear(); // 复用缓冲区

            // 根据 Action 类型分发到 OrderBook 对应方法
            std::visit(
                [&](const auto& cmd) {
                    using T = std::decay_t<decltype(cmd)>;

                    if constexpr (std::is_same_v<T, hft::SubmitAction>) {
                        book.submit({cmd.id, cmd.side, cmd.type, cmd.price, cmd.quantity}, events);
                    } else if constexpr (std::is_same_v<T, hft::CancelAction>) {
                        book.cancel(cmd.order_id, events);
                    } else if constexpr (std::is_same_v<T, hft::ModifyAction>) {
                        book.modify(cmd.order_id, cmd.new_price, cmd.new_qty, events);
                    } else if constexpr (std::is_same_v<T, hft::MarketAction>) {
                        book.market({cmd.id, cmd.side, hft::OrderType::GFD, 0, cmd.qty}, events);
                    } else if constexpr (std::is_same_v<T, hft::PrintAction>) {
                        book.print(events);
                    }
                },
                *action);

            // 输出所有生成的事件
            for (const auto& ev : events) {
                std::cout << hft::format_event(ev) << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "ERROR: " << e.what() << "\n";
        }
    }

    return 0;
}
