#ifndef ORDER_RECORD_H
#define ORDER_RECORD_H

#include <protocol/ProtocolTypes.h>

#include <atomic>
#include <cstdint>
#include <stop_token>
#include <string>

struct OrderRecord {
    int order_id{};
    std::string client_ip;
    std::uint16_t client_port{};
    protocol::Items items{};
    std::stop_source stop_source{};
    // Ensures reserved products are either completed or restored to stock at most once.
    std::atomic<bool> finalized{false};
};

#endif
