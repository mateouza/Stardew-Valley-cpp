#ifndef SESSION_CHANNEL_H
#define SESSION_CHANNEL_H

#include <atomic>
#include <stdexcept>

#include "queue.h"
#include "server_message.h"
#include "session_id.h"

class SessionChannel {
private:
    const SessionId session_id;
    Queue<ServerMessage> outgoing_queue;
    std::atomic<bool> closing{false};

public:
    SessionChannel(SessionId session_id, unsigned int capacity):
            session_id(session_id), outgoing_queue(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument(
                    "La capacidad de la cola de salida debe ser mayor que cero");
        }
    }

    SessionId get_session_id() const { return session_id; }

    Queue<ServerMessage>& get_outgoing_queue() { return outgoing_queue; }

    void request_close() { closing.store(true); }

    bool is_closing() const { return closing.load(); }
};

#endif