#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <protocol/ProtocolTypes.h>

#include <csignal>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

extern volatile std::sig_atomic_t g_shutdown_requested;

extern "C" void handle_shutdown_signal(int sig);

int compute_prep_time(const protocol::Items& items) noexcept;

std::string format_items(const protocol::Items& items);

std::optional<std::uint16_t> parse_port(int argc, char* argv[]);

bool send_all(int fd, std::string_view data);

#endif
