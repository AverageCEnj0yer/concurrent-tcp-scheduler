#ifndef COFFEE_SHOP_SERVER_H
#define COFFEE_SHOP_SERVER_H

#include <concurrency/ThreadPool.h>
#include <protocol/Protocol.h>
#include <server/ClientSession.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

class CoffeeShopServer {
public:
    explicit CoffeeShopServer(std::size_t worker_count);

    int run(std::uint16_t port);

private:
    static protocol::Items generate_initial_stock();

    void log(std::string_view endpoint, std::string_view message);

    void send_error(const std::shared_ptr<ClientSession>& session,
                    protocol::ErrorCode code,
                    std::string_view message);

    std::vector<protocol::LowStockMessage> collect_low_stock_all_locked() const;

    void notify_provider(const std::vector<protocol::LowStockMessage>& alerts);

    void handle_menu_request(const std::shared_ptr<ClientSession>& session);

    void handle_provider_identify(const std::shared_ptr<ClientSession>& session);

    void handle_delivery(const std::shared_ptr<ClientSession>& session, const protocol::Items& items);

    void handle_order_request(const std::shared_ptr<ClientSession>& session, const protocol::Items& items);

    void dispatch_line(const std::shared_ptr<ClientSession>& session, const std::string& line);

    void disconnect_session(const std::shared_ptr<ClientSession>& session, std::string_view reason);

    void handle_client(const std::shared_ptr<ClientSession>& session);

    ThreadPool pool_;

    std::mutex stock_mutex_;
    protocol::Items stock_{};
    int next_order_id_{1};

    std::mutex provider_mutex_;
    std::shared_ptr<ClientSession> active_provider_;

    std::mutex sessions_mutex_;
    std::vector<std::weak_ptr<ClientSession>> sessions_;

    std::mutex log_mutex_;
};

#endif
