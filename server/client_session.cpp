#include "client_session.h"

#include <utility>

ClientSession::ClientSession(Socket&& socket): socket(std::move(socket)) {}
