#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H

#include <protocol/Protocol.h>
#include <server/NetUtils.h>
#include <server/OrderRecord.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct ClientSession {
    int fd{-1};
    std::string ip;
    std::uint16_t port{};
    std::string endpoint;

    std::atomic<bool> is_provider{false};

    std::mutex send_mutex;
    bool send_closed{false};

    std::mutex orders_mutex;
    bool disconnected{false};
    std::vector<std::shared_ptr<OrderRecord>> active_orders;

    ClientSession(int socket_fd, std::string client_ip, std::uint16_t client_port);

    template <typename MessageT>
    bool send_json(const MessageT& msg) {
        std::string payload = protocol::json(msg).dump() + "\n";
        std::lock_guard lock{send_mutex};
        if (send_closed || fd < 0) {
            return false;
        }
        return send_all(fd, payload);
    }

    void close_socket();
};

#endif
