#include "hft/io/parser.hpp"
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace hft {

namespace {

// 字符串转大写（支持大小写不敏感的命令解析）
std::string upper(std::string s) {
    for (auto& c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

} // namespace

std::optional<Action> Parser::parse(const std::string& line) const {
    // 跳过前导空格
    size_t start = 0;
    while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start])))
        ++start;

    // 空行或注释行 → 无 Action
    if (start >= line.size())
        return std::nullopt;
    if (line[start] == '#')
        return std::nullopt;

    std::istringstream ss(line.substr(start));
    std::string cmd;
    ss >> cmd;
    cmd = upper(cmd);

    // ── SUBMIT 命令 ──────────────────────────────────────
    // 格式：SUBMIT <id> <BUY|SELL> <price> <qty> [GFD|IOC|FOK]
    if (cmd == "SUBMIT") {
        SubmitAction a;
        std::string side_str;
        ss >> a.id >> side_str >> a.price >> a.quantity;
        if (ss.fail())
            throw std::runtime_error("Malformed SUBMIT: " + line);

        side_str = upper(side_str);
        if (side_str == "BUY")
            a.side = Side::Buy;
        else if (side_str == "SELL")
            a.side = Side::Sell;
        else
            throw std::runtime_error("Unknown side: " + side_str);

        // 可选：订单类型（缺省 GFD）；显式给出时只接受三种，未知类型必须报错
        std::string type_str;
        if (ss >> type_str) {
            type_str = upper(type_str);
            if (type_str == "GFD")
                a.type = OrderType::GFD;
            else if (type_str == "IOC")
                a.type = OrderType::IOC;
            else if (type_str == "FOK")
                a.type = OrderType::FOK;
            else
                throw std::runtime_error("Unknown order type: " + type_str);
        }

        if (a.price <= 0)
            throw std::runtime_error("Invalid price");
        if (a.quantity == 0)
            throw std::runtime_error("Invalid quantity");
        return Action{a};
    }

    // ── CANCEL 命令 ──────────────────────────────────────
    // 格式：CANCEL <id>
    if (cmd == "CANCEL") {
        CancelAction a;
        ss >> a.order_id;
        if (ss.fail())
            throw std::runtime_error("Malformed CANCEL: " + line);
        return Action{a};
    }

    // ── MODIFY 命令 ──────────────────────────────────────
    // 格式：MODIFY <id> <new_price> <new_qty>
    if (cmd == "MODIFY") {
        ModifyAction a;
        ss >> a.order_id >> a.new_price >> a.new_qty;
        if (ss.fail())
            throw std::runtime_error("Malformed MODIFY: " + line);
        if (a.new_price <= 0)
            throw std::runtime_error("Invalid price");
        if (a.new_qty == 0)
            throw std::runtime_error("Invalid quantity");
        return Action{a};
    }

    // ── MARKET 命令 ──────────────────────────────────────
    // 格式：MARKET <id> <BUY|SELL> <qty>
    if (cmd == "MARKET") {
        MarketAction a;
        std::string side_str;
        ss >> a.id >> side_str >> a.qty;
        if (ss.fail())
            throw std::runtime_error("Malformed MARKET: " + line);

        side_str = upper(side_str);
        if (side_str == "BUY")
            a.side = Side::Buy;
        else if (side_str == "SELL")
            a.side = Side::Sell;
        else
            throw std::runtime_error("Unknown side: " + side_str);

        if (a.qty == 0)
            throw std::runtime_error("Invalid quantity");
        return Action{a};
    }

    // ── PRINT 命令 ───────────────────────────────────────
    if (cmd == "PRINT") {
        return Action{PrintAction{}};
    }

    throw std::runtime_error("Unknown command: " + cmd);
}

} // namespace hft
