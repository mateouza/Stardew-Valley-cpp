#ifndef SDL_CLIENT_H
#define SDL_CLIENT_H

#include <string>

#include "game_info.h"
#include "game_types.h"
#include "network_client.h"

int ejecutar_cliente_sdl(NetworkClient& cliente, game::PlayerId id_propio,
                         const game::GameInfo& info_partida, const std::string& codigo_invitacion);

#endif
