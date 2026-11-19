#include <server/CoffeeShopServer.h>
#include <server/NetUtils.h>
#include <server/ServerConfig.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>

CoffeeShopServer::CoffeeShopServer(std::size_t worker_count)
    : pool_(worker_count),
      stock_(generate_initial_stock()) {}

int CoffeeShopServer::run(std::uint16_t port) {
    const int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        std::perror("socket");
        return 1;
    }

    const int reuse = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::perror("setsockopt(SO_REUSEADDR)");
        close(listen_fd);
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) < 0) {
        std::perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {
        std::perror("listen");
        close(listen_fd);
        return 1;
    }

    {
        std::lock_guard lock{stock_mutex_};
        log("server", "Listening on port " + std::to_string(port) +
            " | Initial stock: " + format_items(stock_));
    }

    struct ClientWorker {
        std::shared_ptr<std::atomic<bool>> done{std::make_shared<std::atomic<bool>>(false)};
        std::jthread thread;
    };
    std::vector<ClientWorker> client_workers;

    while (true) {
        std::erase_if(client_workers, [](const ClientWorker& w) {
            return w.done->load();
        });

        pollfd pfd{};
        pfd.fd = listen_fd;
        pfd.events = POLLIN;
        const int ready = poll(&pfd, 1, POLL_SLICE_MS);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("poll(listen)");
            break;
        }
        if (ready == 0 || (pfd.revents & POLLIN) == 0) {
            continue;
        }

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int client_fd = accept(
            listen_fd,
            reinterpret_cast<sockaddr*>(&client_addr),
            &client_len
        );
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::perror("accept");
            continue;
        }

        char ip_buf[INET_ADDRSTRLEN]{};
        if (inet_ntop(AF_INET, &client_addr.sin_addr, ip_buf, sizeof(ip_buf)) == nullptr) {
            std::snprintf(ip_buf, sizeof(ip_buf), "unknown");
        }
        const std::uint16_t client_port = ntohs(client_addr.sin_port);
        auto session = std::make_shared<ClientSession>(client_fd, std::string(ip_buf), client_port);

        {
            std::lock_guard lock{sessions_mutex_};
            sessions_.push_back(session);
        }

        log(session->endpoint, "Client connected");

        auto done_flag = std::make_shared<std::atomic<bool>>(false);
        client_workers.push_back(ClientWorker{
            done_flag,
            std::jthread([this, session, done_flag]() {
                handle_client(session);
                done_flag->store(true);
            })
        });
    }

    close(listen_fd);
    client_workers.clear();
    log("server", "Server stopped");
    return 0;
}

protocol::Items CoffeeShopServer::generate_initial_stock() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(INITIAL_STOCK_MIN, INITIAL_STOCK_MAX);
    return protocol::Items{dist(gen), dist(gen), dist(gen)};
}

void CoffeeShopServer::log(std::string_view endpoint, std::string_view message) {
    std::lock_guard lock{log_mutex_};
    std::cout << "[" << endpoint << "] " << message << "\n";
}

void CoffeeShopServer::send_error(const std::shared_ptr<ClientSession>& session,
                                  protocol::ErrorCode code,
                                  std::string_view message) {
    log(session->endpoint,
        "ERROR (" + std::string(protocol::to_string(code)) + "): " + std::string(message));
    const protocol::ErrorMessage err{
        protocol::MessageType::ERROR,
        code,
        std::string(message)
    };
    session->send_json(err);
}

void CoffeeShopServer::handle_menu_request(const std::shared_ptr<ClientSession>& session) {
    protocol::Items snapshot{};
    {
        std::lock_guard lock{stock_mutex_};
        snapshot = stock_;
    }
    session->send_json(protocol::ItemsMessage{
        protocol::MessageType::MENU_RESPONSE,
        snapshot
    });
}

void CoffeeShopServer::dispatch_line(const std::shared_ptr<ClientSession>& session, const std::string& line) {
    try {
        const auto j = protocol::json::parse(line);
        const auto msg = protocol::deserialize_message(j);

        std::visit([&](const auto& m) {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T, protocol::EmptyMessage>) {
                switch (m.type) {
                    case protocol::MessageType::MENU_REQUEST:
                        handle_menu_request(session);
                        break;
                    default:
                        send_error(session, protocol::ErrorCode::INVALID_MESSAGE,
                            "Unexpected message type");
                        break;
                }
            } else {
                send_error(session, protocol::ErrorCode::INVALID_MESSAGE,
                    "Clients cannot send server-only message types");
            }
        }, msg);
    } catch (const protocol::ProtocolException& e) {
        send_error(session, e.code(), e.what());
    } catch (const protocol::json::parse_error& e) {
        send_error(session, protocol::ErrorCode::INVALID_MESSAGE, e.what());
    }
}

void CoffeeShopServer::disconnect_session(const std::shared_ptr<ClientSession>& session, std::string_view reason) {
    bool already_disconnected = false;
    {
        std::lock_guard lock{session->orders_mutex};
        already_disconnected = session->disconnected;
        session->disconnected = true;
    }

    if (!already_disconnected) {
        log(session->endpoint, "Disconnected: " + std::string(reason));
    }

    session->close_socket();

    {
        std::lock_guard lock{sessions_mutex_};
        std::erase_if(sessions_, [&session](const std::weak_ptr<ClientSession>& w) {
            auto locked = w.lock();
            return !locked || locked == session;
        });
    }
}

void CoffeeShopServer::handle_client(const std::shared_ptr<ClientSession>& session) {
    std::string line_buffer;
    std::array<char, RECV_CHUNK_BYTES> chunk{};
    bool discarding_oversized_line = false;
    std::string disconnect_reason = "Connection closed";

    while (true) {
        pollfd pfd{};
        pfd.fd = session->fd;
        pfd.events = POLLIN;

        const int ready = poll(&pfd, 1, POLL_SLICE_MS);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            disconnect_reason = std::string("poll error: ") + std::strerror(errno);
            break;
        }
        if (ready == 0) {
            continue;
        }
        if ((pfd.revents & (POLLERR | POLLNVAL)) != 0 && (pfd.revents & POLLIN) == 0) {
            disconnect_reason = "Socket error";
            break;
        }

        const ssize_t bytes_read = recv(session->fd, chunk.data(), chunk.size(), 0);
        if (bytes_read == 0) {
            disconnect_reason = "Client closed connection";
            break;
        }
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            disconnect_reason = std::string("recv error: ") + std::strerror(errno);
            break;
        }

        line_buffer.append(chunk.data(), static_cast<std::size_t>(bytes_read));

        while (true) {
            const std::size_t nl = line_buffer.find('\n');
            if (nl == std::string::npos) {
                if (line_buffer.size() > MAX_LINE_BYTES) {
                    if (!discarding_oversized_line) {
                        send_error(session, protocol::ErrorCode::INVALID_MESSAGE,
                            "Message exceeds maximum allowed length");
                        discarding_oversized_line = true;
                    }
                    line_buffer.clear();
                }
                break;
            }

            if (discarding_oversized_line) {
                line_buffer.erase(0, nl + 1);
                discarding_oversized_line = false;
                continue;
            }

            if (nl > MAX_LINE_BYTES) {
                line_buffer.erase(0, nl + 1);
                send_error(session, protocol::ErrorCode::INVALID_MESSAGE,
                    "Message exceeds maximum allowed length");
                continue;
            }

            std::string line = line_buffer.substr(0, nl);
            line_buffer.erase(0, nl + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            dispatch_line(session, line);
        }
    }

    disconnect_session(session, disconnect_reason);
}
