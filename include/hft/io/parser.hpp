#pragma once

#include "hft/core/action.hpp"
#include <optional>
#include <string>

namespace hft {

class Parser {
public:
    // Returns nullopt for empty lines and comment lines (starting with '#').
    // Returns Action on successful parse.
    // Throws std::runtime_error on malformed input.
    [[nodiscard]] std::optional<Action> parse(const std::string& line) const;
};

} // namespace hft
