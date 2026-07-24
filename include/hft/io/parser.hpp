#pragma once

#include "hft/core/action.hpp"
#include <optional>
#include <string>

namespace hft {

// 命令解析器：将 CLI 文本命令转为内部 Action
class Parser {
public:
    // 解析一行文本命令
    // 返回 nullopt：空行或注释行（以 # 开头）
    // 返回 Action：解析成功
    // 抛出 std::runtime_error：格式错误
    [[nodiscard]] std::optional<Action> parse(const std::string& line) const;
};

} // namespace hft
