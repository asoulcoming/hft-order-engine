#include "hft/book/order_book.hpp"
#include "hft/io/parser.hpp"
#include "hft/io/formatter.hpp"
#include <iostream>
#include <string>
#include <vector>

int main() {
    hft::OrderBook book;
    hft::Parser parser;
    std::string line;
    std::vector<hft::Event> events;

    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        auto action = parser.parse(line);
        if (!action) continue;

        events.clear();
        std::visit([&](const auto& cmd) {
            using T = std::decay_t<decltype(cmd)>;

            if constexpr (std::is_same_v<T, hft::SubmitAction>) {
                book.submit({cmd.id, cmd.side, cmd.type, cmd.price, cmd.quantity}, events);
            } else if constexpr (std::is_same_v<T, hft::CancelAction>) {
                book.cancel(cmd.order_id, events);
            } else if constexpr (std::is_same_v<T, hft::ModifyAction>) {
                book.modify(cmd.order_id, cmd.new_price, cmd.new_qty, events);
            } else if constexpr (std::is_same_v<T, hft::MarketAction>) {
                book.market({cmd.id, cmd.side, hft::OrderType::GTC, 0, cmd.qty}, events);
            } else if constexpr (std::is_same_v<T, hft::PrintAction>) {
                book.print(events);
            }
        }, *action);

        for (const auto& ev : events) {
            std::cout << hft::format_event(ev) << "\n";
        }
    }

    return 0;
}
