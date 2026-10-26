#include "fake_lobby_client.h"

#include <algorithm>
#include <utility>

#include "lobby_validation.h"

namespace lobby {

namespace {

bool is_valid_appearance(const CharacterAppearance& appearance) {
    return appearance.skin_tone < SKIN_TONE_COUNT && appearance.hair_style < HAIR_STYLE_COUNT &&
           appearance.hair_color < HAIR_COLOR_COUNT && appearance.shirt_color < SHIRT_COLOR_COUNT &&
           appearance.pants_color < PANTS_COLOR_COUNT;
}

std::optional<LobbyError> validate_character(const CharacterDraft& character) {
    if (!is_valid_character_name(character.name)) {
        return LobbyError::InvalidCharacterName;
    }

    if (!is_valid_appearance(character.appearance)) {
        return LobbyError::InvalidAppearance;
    }

    return std::nullopt;
}

std::string normalize_invite_code(const std::string& invite_code) {
    std::string normalized = trim_spaces(invite_code);
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
    if (trim_spaces(host).empty() || !is_valid_port(port)) {
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

    const std::string name = trim_spaces(game_name);
    if (!is_valid_game_name(name)) {
        return LobbyError::InvalidGameName;
    }

    if (!has_world(world_id)) {
        return LobbyError::UnknownWorld;
    }

    CharacterDraft draft{trim_spaces(character.name), character.appearance};
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

    if (is_member(*game)) {
        return LobbyError::AlreadyJoined;
    }

    if (game->characters.size() >= MAX_PLAYERS_PER_GAME) {
        return LobbyError::GameFull;
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

    CharacterDraft draft{trim_spaces(character.name), character.appearance};
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
