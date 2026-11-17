#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <chrono>
#include <cstddef>
#include <cstdint>

constexpr std::uint16_t DEFAULT_PORT = 8000;
constexpr auto HEARTBEAT_TIMEOUT = std::chrono::seconds(3);
constexpr int POLL_SLICE_MS = 500;
constexpr std::size_t RECV_CHUNK_BYTES = 4096;
constexpr std::size_t MAX_LINE_BYTES = 8192;

constexpr int COFFEE_PREP_SECONDS = 4;
constexpr int TEA_PREP_SECONDS = 3;
constexpr int CROISSANT_PREP_SECONDS = 2;
constexpr int LOW_STOCK_THRESHOLD = 5;
constexpr int INITIAL_STOCK_MIN = 10;
constexpr int INITIAL_STOCK_MAX = 50;

#endif
