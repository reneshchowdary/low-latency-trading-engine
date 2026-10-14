#include "trading/order_book.hpp"
#include "trading/protocol.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
namespace {
constexpr std::size_t max_line = 1024;
constexpr std::size_t max_output = 65536;
struct Client { std::string input; std::string output; };
bool nonblocking(int fd) {
    const int flags = fcntl(fd, F_GETFL);
    return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}
std::string handle(std::string_view line, trading::OrderBook& book,
                   std::uint64_t& submitted, std::uint64_t& fills) {
    const auto command = trading::parse_command(line);
    if (!command) return "ERROR invalid-command\n";
    switch (command->kind) {
        case trading::Command::Kind::Submit: {
            ++submitted;
            auto result = book.submit(command->order);
            fills += result.fills.size();
            return trading::format_result(result);
        }
        case trading::Command::Kind::Cancel:
            return book.cancel(command->cancel_id) == trading::Status::Cancelled ?
                "CANCELLED\n" : "NOT_FOUND\n";
        case trading::Command::Kind::Top: {
            const auto bid = book.best_bid();
            const auto ask = book.best_ask();
            return "TOP bid=" + (bid ? std::to_string(*bid) : "-") +
                " ask=" + (ask ? std::to_string(*ask) : "-") + "\n";
        }
        case trading::Command::Kind::Stats:
            return "STATS submitted=" + std::to_string(submitted) +
                " fills=" + std::to_string(fills) +
                " resting=" + std::to_string(book.resting_count()) + "\n";
        case trading::Command::Kind::Quit: return "BYE\n";
    }
    return "ERROR\n";
}
}
int main(int argc, char** argv) {
    std::signal(SIGPIPE, SIG_IGN);
    const int port = argc > 1 ? std::atoi(argv[1]) : 9000;
    if (port < 1 || port > 65535) { std::cerr << "invalid port\n"; return 1; }
    const int listener = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (listener < 0) { std::perror("socket"); return 1; }
    const int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<std::uint16_t>(port));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // safe local demonstration by default
    if (bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
        listen(listener, 128) != 0) { std::perror("bind/listen"); close(listener); return 1; }
    std::cout << "trading gateway listening on 127.0.0.1:" << port << std::endl;
    trading::OrderBook book;
    std::map<int, Client> clients;
    std::uint64_t submitted = 0, fills = 0;
    for (;;) {
        std::vector<pollfd> fds;
        fds.push_back({listener, POLLIN, 0});
        for (const auto& [fd, client] : clients)
            fds.push_back({fd, static_cast<short>(POLLIN | (client.output.empty() ? 0 : POLLOUT)), 0});
        const int ready = poll(fds.data(), static_cast<nfds_t>(fds.size()), -1);
        if (ready < 0) { if (errno == EINTR) continue; std::perror("poll"); break; }
        if (fds[0].revents & POLLIN) {
            for (;;) {
                const int fd = accept4(listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
                if (fd < 0) { if (errno != EAGAIN && errno != EWOULDBLOCK) std::perror("accept"); break; }
                if (clients.size() >= 256) close(fd);
                else clients.emplace(fd, Client{});
            }
        }
        for (std::size_t i = 1; i < fds.size(); ++i) {
            const int fd = fds[i].fd;
            auto it = clients.find(fd);
            if (it == clients.end()) continue;
            auto& client = it->second;
            bool disconnect = (fds[i].revents & (POLLERR | POLLNVAL)) != 0;
            if (!disconnect && (fds[i].revents & POLLIN)) {
                char buffer[4096];
                for (;;) {
                    const ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
                    if (n == 0) { disconnect = true; break; }
                    if (n < 0) {
                        if (errno == EINTR) continue;
                        if (errno != EAGAIN && errno != EWOULDBLOCK) disconnect = true;
                        break;
                    }
                    client.input.append(buffer, static_cast<std::size_t>(n));
                    if (client.input.size() > max_line * 4) { disconnect = true; break; }
                    for (;;) {
                        const auto newline = client.input.find('\n');
                        if (newline == std::string::npos) break;
                        auto line = client.input.substr(0, newline);
                        client.input.erase(0, newline + 1);
                        if (line.size() > max_line) { disconnect = true; break; }
                        if (!line.empty() && line.back() == '\r') line.pop_back();
                        client.output += handle(line, book, submitted, fills);
                        if (client.output.size() > max_output) { disconnect = true; break; }
                    }
                    if (disconnect || client.input.size() > max_line) { disconnect = true; break; }
                }
            }
            if (!disconnect && !client.output.empty()) {
                const ssize_t n = send(fd, client.output.data(), client.output.size(), MSG_NOSIGNAL);
                if (n > 0) client.output.erase(0, static_cast<std::size_t>(n));
                else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
                    disconnect = true;
            }
            if (disconnect || (fds[i].revents & POLLHUP)) {
                close(fd);
                clients.erase(it);
            }
        }
    }
    for (const auto& [fd, client] : clients) { (void)client; close(fd); }
    close(listener);
    return 1;
}
