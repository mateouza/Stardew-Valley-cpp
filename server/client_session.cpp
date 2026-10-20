#include "client_session.h"

#include <stdexcept>
#include <utility>

#include <sys/socket.h>

ClientSession::ClientSession(Socket&& socket): socket(std::move(socket)), protocol(this->socket) {}

ClientSession::~ClientSession() {
    stop();
    join();
}

void ClientSession::start(Queue<MatchCommand>& commands, const std::string& name,
                          std::shared_ptr<SessionChannel> channel) {
    if (started) {
        throw std::logic_error("La sesion ya fue iniciada");
    }

    if (stopped) {
        throw std::logic_error("La sesion ya fue detenida");
    }

    if (!channel || channel->is_closing()) {
        throw std::invalid_argument("La sesion debe tener un canal abierto");
    }

    this->channel = std::move(channel);

    try {
        receiver = std::make_unique<ClientReceiver>(protocol, commands, this->channel);

        if (!commands.try_push(AddPlayerCommand{name, this->channel})) {
            throw std::runtime_error("La cola de comandos de la partida esta llena");
        }

        receiver->start();
        started = true;
    } catch (...) {
        stop();
        throw;
    }
}

void ClientSession::stop() {
    if (stopped) {
        return;
    }

    stopped = true;

    if (channel) {
        channel->request_close();
    }

    if (started) {
        receiver->stop();
    }

    try {
        socket.shutdown(SHUT_RDWR);
    } catch (...) {
        failure = std::current_exception();
    }
}

void ClientSession::join() {
    if (started && !joined) {
        receiver->join();
        joined = true;
    }
}

bool ClientSession::has_finished() const {
    if (!started) {
        return stopped;
    }

    return !receiver->is_alive();
}

void ClientSession::rethrow_if_failed() const {
    if (started && !joined) {
        throw std::logic_error("Se debe esperar el cierre de la sesion antes de consultar errores");
    }

    if (failure) {
        std::rethrow_exception(failure);
    }

    if (started) {
        receiver->rethrow_if_failed();
    }
}