#include "trading/protocol.hpp"
#include <charconv>
#include <sstream>
#include <string>
#include <vector>
namespace trading {
namespace {
template<class T> bool number(std::string_view value, T& out) {
    if (value.empty()) return false;
    const auto* begin = value.data();
    const auto* end = begin + value.size();
    const auto [ptr, ec] = std::from_chars(begin, end, out);
    return ec == std::errc{} && ptr == end;
}
std::vector<std::string_view> tokens(std::string_view s) {
    std::vector<std::string_view> parts;
    while (!s.empty()) {
        const auto pos = s.find(' ');
        if (pos == 0) { s.remove_prefix(1); continue; }
        parts.push_back(s.substr(0, pos));
        if (pos == s.npos) break;
        s.remove_prefix(pos + 1);
    }
    return parts;
}
}
std::optional<Command> parse_command(std::string_view line) {
    const auto parts = tokens(line);
    if (parts.empty()) return std::nullopt;
    Command cmd;
    if (parts[0] == "TOP" && parts.size() == 1) { cmd.kind = Command::Kind::Top; return cmd; }
    if (parts[0] == "STATS" && parts.size() == 1) { cmd.kind = Command::Kind::Stats; return cmd; }
    if (parts[0] == "QUIT" && parts.size() == 1) { cmd.kind = Command::Kind::Quit; return cmd; }
    if (parts[0] == "CANCEL" && parts.size() == 2) {
        cmd.kind = Command::Kind::Cancel;
        if (!number(parts[1], cmd.cancel_id) || !cmd.cancel_id) return std::nullopt;
        return cmd;
    }
    if (parts[0] != "ORDER" || parts.size() != 6) return std::nullopt;
    cmd.kind = Command::Kind::Submit;
    if (!number(parts[1], cmd.order.id) || !cmd.order.id) return std::nullopt;
    if (parts[2] == "BUY") cmd.order.side = Side::Buy;
    else if (parts[2] == "SELL") cmd.order.side = Side::Sell;
    else return std::nullopt;
    if (parts[3] == "LIMIT") cmd.order.type = Type::Limit;
    else if (parts[3] == "MARKET") cmd.order.type = Type::Market;
    else return std::nullopt;
    if (!number(parts[4], cmd.order.price) || !number(parts[5], cmd.order.quantity) ||
        !cmd.order.quantity || (cmd.order.type == Type::Limit && cmd.order.price <= 0))
        return std::nullopt;
    return cmd;
}
std::string format_result(const Result& result) {
    std::ostringstream out;
    switch (result.status) {
        case Status::Accepted: out << "ACCEPTED"; break;
        case Status::Rejected: out << "REJECTED"; break;
        case Status::Filled: out << "FILLED"; break;
        case Status::PartiallyFilled: out << "PARTIAL"; break;
        default: out << "UNKNOWN"; break;
    }
    out << " remaining=" << result.remaining << " fills=" << result.fills.size() << "\n";
    for (const auto& f : result.fills)
        out << "TRADE aggressor=" << f.aggressor << " resting=" << f.resting
            << " price=" << f.price << " qty=" << f.quantity << "\n";
    return out.str();
}
}
