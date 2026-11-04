#include "network_receiver.h"

#include <exception>
#include <stdexcept>

NetworkReceiver::NetworkReceiver(ClientProtocol& protocol, Queue<ServerMessage>& messages,
                                 ConnectionCloser& closer):
        protocol(protocol), messages(messages), closer(closer) {}

void NetworkReceiver::run() {
    std::exception_ptr error;

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
    } catch (...) {
        error = std::current_exception();
    }

    closer.close(error);
}
