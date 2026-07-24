#pragma once

#include "hft/core/event.hpp"
#include <string>

namespace hft {

// 事件格式化：将内部 Event 转为可读的单行文本
[[nodiscard]] std::string format_event(const Event& event);

} // namespace hft
