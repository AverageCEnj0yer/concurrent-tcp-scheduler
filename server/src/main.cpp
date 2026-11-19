#include <server/CoffeeShopServer.h>
#include <server/NetUtils.h>

#include <csignal>
#include <cstddef>
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    std::signal(SIGPIPE, SIG_IGN);

    const auto port = parse_port(argc, argv);
    if (!port) {
        std::cerr << "Usage: " << (argc > 0 ? argv[0] : "server") << " [port (1-65535)]\n";
        return 1;
    }

    const unsigned int hw = std::thread::hardware_concurrency();
    const std::size_t worker_count = hw > 0 ? static_cast<std::size_t>(hw) : 4;

    CoffeeShopServer server(worker_count);
    return server.run(*port);
}
