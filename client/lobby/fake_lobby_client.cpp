#include "fake_lobby_client.h"

#include <algorithm>
#include <string_view>
#include <utility>

namespace lobby {

namespace {

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return "";
    }

    const auto last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

std::size_t utf8_length(std::string_view text) {
    return std::count_if(text.begin(), text.end(),
                         [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; });
}

bool is_valid_name(std::string_view name, std::size_t max_length) {
    const bool has_control_characters = std::any_of(
            name.begin(), name.end(), [](char c) { return static_cast<unsigned char>(c) < 0x20; });

    return !name.empty() && utf8_length(name) <= max_length && !has_control_characters;
}

bool is_username_character(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

bool is_valid_username(std::string_view username) {
    return !username.empty() && username.size() <= MAX_USERNAME_LENGTH &&
           std::all_of(username.begin(), username.end(), is_username_character);
}

bool is_valid_port(std::string_view port) {
    return !port.empty() && port.size() <= 5 &&
           std::all_of(port.begin(), port.end(), [](char c) { return c >= '0' && c <= '9'; });
}

bool is_valid_appearance(const CharacterAppearance& appearance) {
    return appearance.skin_tone < SKIN_TONE_COUNT && appearance.hair_style < HAIR_STYLE_COUNT &&
           appearance.hair_color < HAIR_COLOR_COUNT && appearance.shirt_color < SHIRT_COLOR_COUNT &&
           appearance.pants_color < PANTS_COLOR_COUNT;
}

std::optional<LobbyError> validate_character(const CharacterDraft& character) {
    if (!is_valid_name(character.name, MAX_CHARACTER_NAME_LENGTH)) {
        return LobbyError::InvalidCharacterName;
    }

    if (!is_valid_appearance(character.appearance)) {
        return LobbyError::InvalidAppearance;
    }

    return std::nullopt;
}

std::string normalize_invite_code(const std::string& invite_code) {
    std::string normalized = trim(invite_code);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](char c) {
        return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
    });
    return normalized;
}

}

FakeLobbyClient::FakeLobbyClient(bool with_demo_games):
        worlds{{"valle_clasico", "Valle Clásico",
                "La granja de siempre: el pueblo al este y la mina al norte."},
               {"orilla_del_rio", "Orilla del Río",
                "Una granja amplia junto al río, ideal para cultivar."},
               {"colinas_altas", "Colinas Altas",
                "Terreno con desniveles y una mina de pasillos estrechos."}} {
    if (with_demo_games) {
        seed_demo_games();
    }
}

void FakeLobbyClient::seed_demo_games() {
    StoredGame& shared_farm =
            store_game("Granja de Pandamos", "valle_clasico", "Pandamos", DEMO_INVITE_CODE);
    shared_farm.characters.emplace("Pandamos",
                                   CharacterDraft{"Pandamos", CharacterAppearance{1, 2, 0, 1, 0}});
    shared_farm.characters.emplace("dipa",
                                   CharacterDraft{"Dipa", CharacterAppearance{3, 1, 3, 4, 2}});

    StoredGame& solo_farm = store_game("Huerta de Dipa", "orilla_del_rio", "dipa", "");
    solo_farm.characters.emplace("dipa",
                                 CharacterDraft{"Dipa", CharacterAppearance{3, 1, 3, 2, 1}});
}

bool FakeLobbyClient::has_world(const std::string& world_id) const {
    return std::any_of(worlds.begin(), worlds.end(),
                       [&world_id](const WorldInfo& world) { return world.id == world_id; });
}

bool FakeLobbyClient::is_member(const StoredGame& game) const {
    return username && game.characters.count(*username) > 0;
}

FakeLobbyClient::StoredGame* FakeLobbyClient::find_by_invite_code(const std::string& invite_code) {
    if (invite_code.empty()) {
        return nullptr;
    }

    auto game = std::find_if(games.begin(), games.end(), [&invite_code](const StoredGame& stored) {
        return stored.invite_code == invite_code;
    });
    return game == games.end() ? nullptr : &*game;
}

FakeLobbyClient::StoredGame* FakeLobbyClient::find_by_id(const std::string& game_id) {
    auto game = std::find_if(games.begin(), games.end(),
                             [&game_id](const StoredGame& stored) { return stored.id == game_id; });
    return game == games.end() ? nullptr : &*game;
}

FakeLobbyClient::StoredGame& FakeLobbyClient::store_game(const std::string& name,
                                                         const std::string& world_id,
                                                         const std::string& owner,
                                                         const std::string& invite_code) {
    games.push_back(StoredGame{"partida-" + std::to_string(next_game_number++),
                               name,
                               world_id,
                               owner,
                               invite_code,
                               {}});
    return games.back();
}

GameTicket FakeLobbyClient::make_ticket(const StoredGame& game) const {
    return GameTicket{game.id, game.name, game.characters.at(*username), !game.invite_code.empty()};
}

std::optional<LobbyError> FakeLobbyClient::connect(const std::string& host, const std::string& port,
                                                   const std::string& requested_username) {
    if (trim(host).empty() || !is_valid_port(trim(port))) {
        return LobbyError::ServerUnavailable;
    }

    if (!is_valid_username(requested_username)) {
        return LobbyError::InvalidUsername;
    }

    username = requested_username;
    return std::nullopt;
}

LobbyResult<std::vector<WorldInfo>> FakeLobbyClient::list_worlds() {
    if (!username) {
        return LobbyError::ServerUnavailable;
    }

    return worlds;
}

LobbyResult<std::vector<SavedGameInfo>> FakeLobbyClient::list_saved_games() {
    if (!username) {
        return LobbyError::ServerUnavailable;
    }

    std::vector<SavedGameInfo> saved_games;
    for (const StoredGame& game: games) {
        if (is_member(game)) {
            saved_games.push_back(SavedGameInfo{game.id, game.name, game.characters.at(*username),
                                                !game.invite_code.empty()});
        }
    }
    return saved_games;
}

LobbyResult<GameTicket> FakeLobbyClient::create_game(const std::string& game_name,
                                                     const std::string& world_id,
                                                     const CharacterDraft& character) {
    if (!username) {
        return LobbyError::ServerUnavailable;
    }

    const std::string name = trim(game_name);
    if (!is_valid_name(name, MAX_GAME_NAME_LENGTH)) {
        return LobbyError::InvalidGameName;
    }

    if (!has_world(world_id)) {
        return LobbyError::UnknownWorld;
    }

    CharacterDraft draft{trim(character.name), character.appearance};
    if (auto error = validate_character(draft)) {
        return *error;
    }

    StoredGame& game = store_game(name, world_id, *username, "");
    game.characters.emplace(*username, std::move(draft));
    return make_ticket(game);
}

LobbyResult<JoinableGameInfo> FakeLobbyClient::find_game(const std::string& invite_code) {
    if (!username) {
        return LobbyError::ServerUnavailable;
    }

    const StoredGame* game = find_by_invite_code(normalize_invite_code(invite_code));
    if (!game) {
        return LobbyError::InvalidInviteCode;
    }

    return JoinableGameInfo{game->id, game->name, game->owner, game->characters.size(),
                            MAX_PLAYERS_PER_GAME};
}

LobbyResult<GameTicket> FakeLobbyClient::join_game(const std::string& invite_code,
                                                   const CharacterDraft& character) {
    if (!username) {
        return LobbyError::ServerUnavailable;
    }

    StoredGame* game = find_by_invite_code(normalize_invite_code(invite_code));
    if (!game) {
        return LobbyError::InvalidInviteCode;
    }

    if (is_member(*game)) {
        return LobbyError::AlreadyJoined;
    }

    if (game->characters.size() >= MAX_PLAYERS_PER_GAME) {
        return LobbyError::GameFull;
    }

    CharacterDraft draft{trim(character.name), character.appearance};
    if (auto error = validate_character(draft)) {
        return *error;
    }

    game->characters.emplace(*username, std::move(draft));
    return make_ticket(*game);
}

LobbyResult<GameTicket> FakeLobbyClient::resume_game(const std::string& game_id) {
    if (!username) {
        return LobbyError::ServerUnavailable;
    }

    const StoredGame* game = find_by_id(game_id);
    if (!game || !is_member(*game)) {
        return LobbyError::UnknownGame;
    }

    return make_ticket(*game);
}

}
