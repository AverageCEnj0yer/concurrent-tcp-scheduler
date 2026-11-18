#include <server/ClientSession.h>

#include <sys/socket.h>
#include <unistd.h>

#include <utility>

ClientSession::ClientSession(int socket_fd, std::string client_ip, std::uint16_t client_port)
    : fd(socket_fd),
      ip(std::move(client_ip)),
      port(client_port),
      endpoint(ip + ":" + std::to_string(port)) {}

void ClientSession::close_socket() {
    std::lock_guard lock{send_mutex};
    send_closed = true;
    if (fd >= 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        fd = -1;
    }
}
