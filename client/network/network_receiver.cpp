#include "network_receiver.h"

#include <stdexcept>

#include "liberror.h"

NetworkReceiver::NetworkReceiver(ClientProtocol& protocol, Queue<ServerMessage>& messages):
        protocol(protocol), messages(messages) {}

void NetworkReceiver::run() {
    try {
        while (should_keep_running()) {
            const auto message = protocol.receive_message();

            if (!message || !should_keep_running()) {
                break;
            }

            if (!messages.try_push(*message)) {
                throw std::runtime_error("La cola de mensajes del cliente esta llena");
            }
        }
    } catch (const ClosedQueue&) {
    } catch (const LibError&) {
        if (should_keep_running()) {
            failure = std::current_exception();
        }
    } catch (...) {
        failure = std::current_exception();
    }

    NetworkReceiver::stop();
}

void NetworkReceiver::stop() {
    Thread::stop();

    std::lock_guard<std::mutex> lock(close_mutex);

    if (!queue_closed) {
        messages.close();
        queue_closed = true;
    }
}

void NetworkReceiver::rethrow_if_failed() const {
    if (failure) {
        std::rethrow_exception(failure);
    }
}
