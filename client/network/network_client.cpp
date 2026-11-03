#include "network_client.h"

#include <stdexcept>
#include <utility>

NetworkClient::NetworkClient(Socket&& socket, unsigned int request_capacity,
                             unsigned int message_capacity):
        socket(std::move(socket)),
        protocol(this->socket),
        requests(request_capacity),
        messages(message_capacity),
        closer(this->socket, requests, messages),
        sender(protocol, requests, closer),
        receiver(protocol, messages, closer) {
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
    if (started || closer.is_closed()) {
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
    if (!started || closer.is_closed()) {
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

std::optional<ServerMessage> NetworkClient::receive() {
    if (!started) {
        throw std::logic_error("La conexion no fue iniciada");
    }

    try {
        return messages.pop();
    } catch (const ClosedQueue&) {
        return std::nullopt;
    }
}

bool NetworkClient::has_finished() const { return closer.is_closed(); }

void NetworkClient::stop() {
    sender.stop();
    receiver.stop();
    closer.close();
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

    closer.rethrow_if_failed();
}
