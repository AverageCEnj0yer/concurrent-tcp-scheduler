#include <server/NetUtils.h>
#include <server/ServerConfig.h>

#include <sys/socket.h>

#include <cerrno>
#include <charconv>
#include <limits>

volatile std::sig_atomic_t g_shutdown_requested = 0;

extern "C" void handle_shutdown_signal(int /*sig*/) {
    g_shutdown_requested = 1;
}

int compute_prep_time(const protocol::Items& items) noexcept {
    return items.coffee * COFFEE_PREP_SECONDS +
           items.tea * TEA_PREP_SECONDS +
           items.croissant * CROISSANT_PREP_SECONDS;
}

std::string format_items(const protocol::Items& items) {
    return "coffee=" + std::to_string(items.coffee) +
           ", tea=" + std::to_string(items.tea) +
           ", croissant=" + std::to_string(items.croissant);
}

std::optional<std::uint16_t> parse_port(int argc, char* argv[]) {
    if (argc < 2) {
        return DEFAULT_PORT;
    }
    if (argc > 2) {
        return std::nullopt;
    }
    const std::string_view arg(argv[1]);
    std::uint32_t value = 0;
    const auto* begin = arg.data();
    const auto* end = arg.data() + arg.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end || value == 0 ||
        value > std::numeric_limits<std::uint16_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(value);
}

bool send_all(int fd, std::string_view data) {
    std::size_t total_sent = 0;
    while (total_sent < data.size()) {
        const ssize_t sent = send(
            fd,
            data.data() + total_sent,
            data.size() - total_sent,
            MSG_NOSIGNAL
        );
        if (sent > 0) {
            total_sent += static_cast<std::size_t>(sent);
            continue;
        }
        if (sent < 0 && errno == EINTR) {
            continue;
        }
        return false;
    }
    return true;
}
