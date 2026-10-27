#include "lobby_error_message.h"

namespace launcher {

QString lobby_error_message(lobby::LobbyError error) {
    switch (error) {
        case lobby::LobbyError::ServerUnavailable:
            return "No se pudo conectar con el servidor. Revisá la dirección y el puerto.";
        case lobby::LobbyError::InvalidUsername:
            return QString("El nombre de usuario debe tener entre 1 y %1 caracteres: letras, "
                           "números o guion bajo.")
                    .arg(lobby::MAX_USERNAME_LENGTH);
        case lobby::LobbyError::InvalidGameName:
            return QString("El nombre de la partida no puede estar vacío ni superar los %1 "
                           "caracteres.")
                    .arg(lobby::MAX_GAME_NAME_LENGTH);
        case lobby::LobbyError::InvalidCharacterName:
            return QString("El nombre del personaje no puede estar vacío ni superar los %1 "
                           "caracteres.")
                    .arg(lobby::MAX_CHARACTER_NAME_LENGTH);
        case lobby::LobbyError::InvalidAppearance:
            return "La apariencia elegida no es válida.";
        case lobby::LobbyError::UnknownWorld:
            return "El mundo elegido ya no está disponible.";
        case lobby::LobbyError::InvalidInviteCode:
            return "No hay ninguna partida con ese código. Revisalo y probá de nuevo.";
        case lobby::LobbyError::AlreadyJoined:
            return "Ya formás parte de esa partida. Entrá desde «Retomar partida».";
        case lobby::LobbyError::GameFull:
            return "La partida está llena.";
        case lobby::LobbyError::UnknownGame:
            return "Esa partida ya no está disponible.";
    }
    return "Ocurrió un error inesperado.";
}

}
