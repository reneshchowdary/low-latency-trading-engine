#pragma once
#include "trading/order_book.hpp"
#include <optional>
#include <string>
#include <string_view>
namespace trading {
struct Command {
    enum class Kind { Submit, Cancel, Top, Stats, Quit };
    Kind kind{Kind::Top};
    Request order{};
    OrderId cancel_id{};
};
[[nodiscard]] std::optional<Command> parse_command(std::string_view line);
[[nodiscard]] std::string format_result(const Result& result);
}
