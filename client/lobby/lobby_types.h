#ifndef CLIENT_LOBBY_LOBBY_TYPES_H_
#define CLIENT_LOBBY_LOBBY_TYPES_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

namespace lobby {

inline constexpr std::size_t MAX_USERNAME_LENGTH = 16;
inline constexpr std::size_t MAX_GAME_NAME_LENGTH = 24;
inline constexpr std::size_t MAX_CHARACTER_NAME_LENGTH = 16;

inline constexpr std::uint8_t SKIN_TONE_COUNT = 5;
inline constexpr std::uint8_t HAIR_STYLE_COUNT = 4;
inline constexpr std::uint8_t HAIR_COLOR_COUNT = 6;
inline constexpr std::uint8_t SHIRT_COLOR_COUNT = 8;
inline constexpr std::uint8_t PANTS_COLOR_COUNT = 6;

struct CharacterAppearance {
    std::uint8_t skin_tone = 0;
    std::uint8_t hair_style = 0;
    std::uint8_t hair_color = 0;
    std::uint8_t shirt_color = 0;
    std::uint8_t pants_color = 0;
};

struct CharacterDraft {
    std::string name;
    CharacterAppearance appearance;
};

struct WorldInfo {
    std::string id;
    std::string name;
    std::string description;
};

struct SavedGameInfo {
    std::string game_id;
    std::string game_name;
    CharacterDraft character;
    bool multiplayer = false;
};

struct JoinableGameInfo {
    std::string game_id;
    std::string game_name;
    std::string owner;
    std::size_t player_count = 0;
    std::size_t max_players = 0;
};

struct GameTicket {
    std::string game_id;
    std::string game_name;
    CharacterDraft character;
    bool multiplayer = false;
};

enum class LobbyError {
    ServerUnavailable,
    InvalidUsername,
    InvalidGameName,
    InvalidCharacterName,
    InvalidAppearance,
    UnknownWorld,
    InvalidInviteCode,
    AlreadyJoined,
    GameFull,
    UnknownGame,
};

template <typename T>
using LobbyResult = std::variant<T, LobbyError>;

}

#endif
