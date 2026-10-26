#include "network_client.h"

#include <stdexcept>
#include <utility>

#include <sys/socket.h>

NetworkClient::NetworkClient(Socket&& socket, unsigned int request_capacity,
                             unsigned int message_capacity):
        socket(std::move(socket)),
        protocol(this->socket),
        requests(request_capacity),
        messages(message_capacity),
        sender(protocol, requests),
        receiver(protocol, messages) {
    if (request_capacity == 0 || message_capacity == 0) {
        throw std::invalid_argument("Las capacidades de las colas deben ser positivas");
    }
}

NetworkClient::NetworkClient(const char* hostname, const char* service,
                             unsigned int request_capacity, unsigned int message_capacity):
        NetworkClient(Socket(hostname, service), request_capacity, message_capacity) {}

NetworkClient::~NetworkClient() {
    stop();
    join();
}

void NetworkClient::start() {
    if (started || stopped) {
        throw std::logic_error("La conexion no puede iniciarse nuevamente");
    }

    try {
        receiver.start();
        receiver_joinable = true;

        sender.start();
        sender_joinable = true;

        started = true;
    } catch (...) {
        stop();
        join();
        throw;
    }
}

bool NetworkClient::try_send(const ClientRequest& request) {
    if (!started || stopped) {
        throw std::logic_error("La conexion no esta activa");
    }

    return requests.try_push(request);
}

std::optional<ServerMessage> NetworkClient::try_receive() {
    if (!started) {
        throw std::logic_error("La conexion no fue iniciada");
    }

    ServerMessage message;

    try {
        if (!messages.try_pop(message)) {
            return std::nullopt;
        }
    } catch (const ClosedQueue&) {
        return std::nullopt;
    }

    return message;
}

bool NetworkClient::has_finished() const {
    if (!started) {
        return stopped;
    }

    return stopped || !sender.is_alive() || !receiver.is_alive();
}

void NetworkClient::stop() {
    if (stopped) {
        return;
    }

    stopped = true;

    sender.stop();
    receiver.stop();

    try {
        socket.shutdown(SHUT_RDWR);
    } catch (...) {
        failure = std::current_exception();
    }
}

void NetworkClient::join() {
    if (sender_joinable) {
        sender.join();
        sender_joinable = false;
    }

    if (receiver_joinable) {
        receiver.join();
        receiver_joinable = false;
    }
}

void NetworkClient::rethrow_if_failed() const {
    if (sender_joinable || receiver_joinable) {
        throw std::logic_error(
                "Se debe esperar el cierre de la conexion antes de consultar errores");
    }

    if (failure) {
        std::rethrow_exception(failure);
    }

    sender.rethrow_if_failed();
    receiver.rethrow_if_failed();
}
