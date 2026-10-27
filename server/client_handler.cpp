#include "client_handler.h"

#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <variant>

#include "liberror.h"

ClientHandler::ClientHandler(Socket&& socket, MatchManager& matches, SessionId session_id,
                             unsigned int outgoing_capacity):
        matches(matches),
        session(std::move(socket)),
        session_id(session_id),
        outgoing_capacity(outgoing_capacity) {
    if (outgoing_capacity == 0) {
        throw std::invalid_argument("La capacidad de salida debe ser mayor que cero");
    }
}

ClientHandler::~ClientHandler() {
    ClientHandler::stop();
    join();
}

void ClientHandler::start() {
    if (started || stopped) {
        throw std::logic_error("El handler no puede iniciarse nuevamente");
    }

    Thread::start();
    started = true;
}

void ClientHandler::run() {
    try {
        while (should_keep_running()) {
            const auto request = session.receive_lobby_request();

            if (!request || !should_keep_running()) {
                break;
            }

            std::optional<ManagedMatch> selected;
            std::string player_name;

            if (const auto* create = std::get_if<CreateMatchRequest>(&*request)) {
                selected = matches.create_match(create->match_name);
                player_name = create->player_name;
            } else {
                const auto& join = std::get<JoinMatchRequest>(*request);
                selected = matches.find_match(join.invitation_code);
                player_name = join.player_name;
            }

            if (!selected) {
                session.send_match_response(MatchResponse{std::nullopt});
                continue;
            }

            if (!should_keep_running()) {
                break;
            }

            session.send_match_response(MatchResponse{selected->invitation_code});

            auto channel = std::make_shared<SessionChannel>(session_id, outgoing_capacity);

            session.start(selected->match->get_commands(), player_name, channel);

            while (should_keep_running() && !session.has_finished()) {
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
            }

            session.stop();
            session.join();
            session.rethrow_if_failed();

            break;
        }
    } catch (const LibError&) {
        if (should_keep_running()) {
            failure = std::current_exception();
        }
    } catch (...) {
        failure = std::current_exception();
    }

    session.stop();
    session.join();

    try {
        session.rethrow_if_failed();
    } catch (...) {
        if (!failure) {
            failure = std::current_exception();
        }
    }
}

void ClientHandler::stop() {
    if (stopped) {
        return;
    }

    stopped = true;
    Thread::stop();

    try {
        session.shutdown_connection();
    } catch (...) {
        shutdown_failure = std::current_exception();
    }
}

void ClientHandler::join() {
    if (started && !joined) {
        Thread::join();
        joined = true;
    }
}

void ClientHandler::rethrow_if_failed() const {
    if (started && !joined) {
        throw std::logic_error("Se debe esperar al handler antes de consultar errores");
    }

    if (failure) {
        std::rethrow_exception(failure);
    }

    if (shutdown_failure) {
        std::rethrow_exception(shutdown_failure);
    }
}
