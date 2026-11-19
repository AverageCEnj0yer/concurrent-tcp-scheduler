#ifndef COFFEE_SHOP_SERVER_H
#define COFFEE_SHOP_SERVER_H

#include <concurrency/ThreadPool.h>
#include <protocol/Protocol.h>
#include <server/ClientSession.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

class CoffeeShopServer {
public:
    explicit CoffeeShopServer(std::size_t worker_count);

    int run(std::uint16_t port);

private:
    static protocol::Items generate_initial_stock();

    void log(std::string_view endpoint, std::string_view message);

    void disconnect_session(const std::shared_ptr<ClientSession>& session, std::string_view reason);

    void handle_client(const std::shared_ptr<ClientSession>& session);

    ThreadPool pool_;

    std::mutex stock_mutex_;
    protocol::Items stock_{};

    std::mutex sessions_mutex_;
    std::vector<std::weak_ptr<ClientSession>> sessions_;

    std::mutex log_mutex_;
};

#endif
