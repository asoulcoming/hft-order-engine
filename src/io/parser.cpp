#include "hft/io/parser.hpp"
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace hft {

namespace {

std::string upper(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

} // namespace

std::optional<Action> Parser::parse(const std::string& line) const {
    size_t start = 0;
    while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start])))
        ++start;

    if (start >= line.size()) return std::nullopt;
    if (line[start] == '#') return std::nullopt;

    std::istringstream ss(line.substr(start));
    std::string cmd;
    ss >> cmd;
    cmd = upper(cmd);

    if (cmd == "SUBMIT") {
        SubmitAction a;
        std::string side_str;
        ss >> a.id >> side_str >> a.price >> a.quantity;
        if (ss.fail()) throw std::runtime_error("Malformed SUBMIT: " + line);

        side_str = upper(side_str);
        if (side_str == "BUY") a.side = Side::Buy;
        else if (side_str == "SELL") a.side = Side::Sell;
        else throw std::runtime_error("Unknown side: " + side_str);

        std::string type_str;
        if (ss >> type_str) {
            type_str = upper(type_str);
            if (type_str == "IOC") a.type = OrderType::IOC;
            else if (type_str == "FOK") a.type = OrderType::FOK;
        }

        if (a.price <= 0) throw std::runtime_error("Invalid price");
        if (a.quantity == 0) throw std::runtime_error("Invalid quantity");
        return Action{a};
    }

    if (cmd == "CANCEL") {
        CancelAction a;
        ss >> a.order_id;
        if (ss.fail()) throw std::runtime_error("Malformed CANCEL: " + line);
        return Action{a};
    }

    if (cmd == "MODIFY") {
        ModifyAction a;
        ss >> a.order_id >> a.new_price >> a.new_qty;
        if (ss.fail()) throw std::runtime_error("Malformed MODIFY: " + line);
        if (a.new_price <= 0) throw std::runtime_error("Invalid price");
        if (a.new_qty == 0) throw std::runtime_error("Invalid quantity");
        return Action{a};
    }

    if (cmd == "MARKET") {
        MarketAction a;
        std::string side_str;
        ss >> a.id >> side_str >> a.qty;
        if (ss.fail()) throw std::runtime_error("Malformed MARKET: " + line);

        side_str = upper(side_str);
        if (side_str == "BUY") a.side = Side::Buy;
        else if (side_str == "SELL") a.side = Side::Sell;
        else throw std::runtime_error("Unknown side: " + side_str);

        if (a.qty == 0) throw std::runtime_error("Invalid quantity");
        return Action{a};
    }

    if (cmd == "PRINT") {
        return Action{PrintAction{}};
    }

    throw std::runtime_error("Unknown command: " + cmd);
}

} // namespace hft
