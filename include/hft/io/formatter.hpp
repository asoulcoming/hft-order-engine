#pragma once

#include "hft/core/event.hpp"
#include <string>

namespace hft {

// Returns a stable, single-line text representation of an event.
[[nodiscard]] std::string format_event(const Event& event);

} // namespace hft
