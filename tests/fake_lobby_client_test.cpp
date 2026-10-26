#include "lobby/fake_lobby_client.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "gtest/gtest.h"

namespace lobby {

class FakeLobbyClientTest: public ::testing::Test {
protected:
    FakeLobbyClient client;

    static CharacterDraft make_character(const std::string& name = "Juana") {
        return CharacterDraft{name, CharacterAppearance{1, 1, 1, 1, 1}};
    }

    template <typename T>
    static std::optional<LobbyError> error_of(const LobbyResult<T>& result) {
        if (const auto* error = std::get_if<LobbyError>(&result)) {
            return *error;
        }
        return std::nullopt;
    }

    void connect_as(const std::string& username) {
        ASSERT_EQ(client.connect("localhost", "8080", username), std::nullopt);
    }

    std::vector<SavedGameInfo> saved_games() {
        return std::get<std::vector<SavedGameInfo>>(client.list_saved_games());
    }
};

TEST_F(FakeLobbyClientTest, RejectsRequestsBeforeConnecting) {
    EXPECT_EQ(error_of(client.list_worlds()), LobbyError::ServerUnavailable);
    EXPECT_EQ(error_of(client.list_saved_games()), LobbyError::ServerUnavailable);
    EXPECT_EQ(error_of(client.create_game("Granja", "valle_clasico", make_character())),
              LobbyError::ServerUnavailable);
    EXPECT_EQ(error_of(client.find_game(FakeLobbyClient::DEMO_INVITE_CODE)),
              LobbyError::ServerUnavailable);
    EXPECT_EQ(error_of(client.join_game(FakeLobbyClient::DEMO_INVITE_CODE, make_character())),
              LobbyError::ServerUnavailable);
    EXPECT_EQ(error_of(client.resume_game("partida-1")), LobbyError::ServerUnavailable);
}

TEST_F(FakeLobbyClientTest, RejectsMissingServerAddress) {
    EXPECT_EQ(client.connect("", "8080", "juana"), LobbyError::ServerUnavailable);
    EXPECT_EQ(client.connect("localhost", "", "juana"), LobbyError::ServerUnavailable);
    EXPECT_EQ(client.connect("localhost", "puerto", "juana"), LobbyError::ServerUnavailable);
}

TEST_F(FakeLobbyClientTest, RejectsPortsOutOfRangeOrNotNumeric) {
    for (const char* port: {"0", "65536", "99999", "abc", "80a", " 8080"}) {
        EXPECT_EQ(client.connect("localhost", port, "juana"), LobbyError::ServerUnavailable)
                << port;
    }
}

TEST_F(FakeLobbyClientTest, AcceptsPortsAtRangeLimits) {
    EXPECT_EQ(client.connect("localhost", "1", "juana"), std::nullopt);
    EXPECT_EQ(client.connect("localhost", "65535", "juana"), std::nullopt);
}

TEST_F(FakeLobbyClientTest, RejectsInvalidUsernames) {
    const std::string usernames[] = {"", "con espacio", "ñandu", "@dipa",
                                     std::string(MAX_USERNAME_LENGTH + 1, 'a')};
    for (const std::string& username: usernames) {
        EXPECT_EQ(client.connect("localhost", "8080", username), LobbyError::InvalidUsername)
                << username;
    }
}

TEST_F(FakeLobbyClientTest, AcceptsUsernamesWithLettersDigitsAndUnderscore) {
    EXPECT_EQ(client.connect("localhost", "8080", "Mate_Capo1"), std::nullopt);
    EXPECT_EQ(client.connect("localhost", "8080", std::string(MAX_USERNAME_LENGTH, 'a')),
              std::nullopt);
}

TEST_F(FakeLobbyClientTest, ListsAvailableWorlds) {
    connect_as("juana");

    const auto worlds = std::get<std::vector<WorldInfo>>(client.list_worlds());

    ASSERT_EQ(worlds.size(), 3u);
    for (const WorldInfo& world: worlds) {
        EXPECT_FALSE(world.id.empty());
        EXPECT_FALSE(world.name.empty());
    }
}

TEST_F(FakeLobbyClientTest, NewUserHasNoSavedGames) {
    connect_as("juana");

    EXPECT_TRUE(saved_games().empty());
}

TEST_F(FakeLobbyClientTest, DemoUserHasSharedAndSoloGames) {
    connect_as("dipa");

    const auto games = saved_games();

    ASSERT_EQ(games.size(), 2u);
    EXPECT_TRUE(games[0].multiplayer);
    EXPECT_FALSE(games[1].multiplayer);
    EXPECT_EQ(games[0].character.name, "Dipa");
}

TEST_F(FakeLobbyClientTest, CreatesSinglePlayerGameAndListsItAsSaved) {
    connect_as("juana");

    const auto ticket = std::get<GameTicket>(
            client.create_game("  Granja Girasol  ", "orilla_del_rio", make_character(" Juana ")));

    EXPECT_EQ(ticket.game_name, "Granja Girasol");
    EXPECT_EQ(ticket.character.name, "Juana");
    EXPECT_FALSE(ticket.multiplayer);

    const auto games = saved_games();
    ASSERT_EQ(games.size(), 1u);
    EXPECT_EQ(games[0].game_id, ticket.game_id);
    EXPECT_FALSE(games[0].multiplayer);

    const auto resumed = std::get<GameTicket>(client.resume_game(ticket.game_id));
    EXPECT_EQ(resumed.game_name, "Granja Girasol");
}

TEST_F(FakeLobbyClientTest, RejectsInvalidGameNames) {
    connect_as("juana");

    const std::string names[] = {"", "   ", std::string(MAX_GAME_NAME_LENGTH + 1, 'g')};
    for (const std::string& name: names) {
        EXPECT_EQ(error_of(client.create_game(name, "valle_clasico", make_character())),
                  LobbyError::InvalidGameName);
    }

    EXPECT_EQ(error_of(client.create_game(std::string(MAX_GAME_NAME_LENGTH, 'g'), "valle_clasico",
                                          make_character())),
              std::nullopt);
}

TEST_F(FakeLobbyClientTest, CountsNameLengthInCharactersNotBytes) {
    connect_as("juana");

    std::string longest_name;
    for (std::size_t i = 0; i < MAX_CHARACTER_NAME_LENGTH; ++i) {
        longest_name += "ñ";
    }

    EXPECT_EQ(error_of(client.create_game("Granja", "valle_clasico", make_character(longest_name))),
              std::nullopt);
    EXPECT_EQ(error_of(client.create_game("Granja", "valle_clasico",
                                          make_character(longest_name + "ñ"))),
              LobbyError::InvalidCharacterName);
}

TEST_F(FakeLobbyClientTest, RejectsMalformedOrUnlistedNames) {
    connect_as("juana");
    const std::string stray_continuations(1000, '\x80');

    EXPECT_EQ(error_of(client.create_game(stray_continuations, "valle_clasico", make_character())),
              LobbyError::InvalidGameName);
    EXPECT_EQ(error_of(client.create_game("Granja 🙂", "valle_clasico", make_character())),
              LobbyError::InvalidGameName);
    EXPECT_EQ(error_of(client.create_game("Granja", "valle_clasico",
                                          make_character(stray_continuations))),
              LobbyError::InvalidCharacterName);
    EXPECT_EQ(error_of(client.create_game("Granja", "valle_clasico", make_character("\tJuana"))),
              LobbyError::InvalidCharacterName);
}

TEST_F(FakeLobbyClientTest, RejectsUnknownWorld) {
    connect_as("juana");

    EXPECT_EQ(error_of(client.create_game("Granja", "desierto", make_character())),
              LobbyError::UnknownWorld);
}

TEST_F(FakeLobbyClientTest, RejectsInvalidCharacters) {
    connect_as("juana");

    EXPECT_EQ(error_of(client.create_game("Granja", "valle_clasico", make_character("  "))),
              LobbyError::InvalidCharacterName);

    CharacterDraft character = make_character();
    character.appearance.skin_tone = SKIN_TONE_COUNT;
    EXPECT_EQ(error_of(client.create_game("Granja", "valle_clasico", character)),
              LobbyError::InvalidAppearance);
}

TEST_F(FakeLobbyClientTest, FindsGameByInviteCodeIgnoringCaseAndSpaces) {
    connect_as("juana");

    const auto game = std::get<JoinableGameInfo>(client.find_game("  valle1 "));

    EXPECT_EQ(game.game_name, "Granja de Pandamos");
    EXPECT_EQ(game.owner, "Pandamos");
    EXPECT_EQ(game.player_count, 2u);
    EXPECT_EQ(game.max_players, FakeLobbyClient::MAX_PLAYERS_PER_GAME);
}

TEST_F(FakeLobbyClientTest, RejectsUnknownInviteCode) {
    connect_as("juana");

    EXPECT_EQ(error_of(client.find_game("NOEXISTE")), LobbyError::InvalidInviteCode);
    EXPECT_EQ(error_of(client.find_game("")), LobbyError::InvalidInviteCode);
    EXPECT_EQ(error_of(client.join_game("NOEXISTE", make_character())),
              LobbyError::InvalidInviteCode);
}

TEST_F(FakeLobbyClientTest, JoinsSharedGameAsMultiplayer) {
    connect_as("juana");

    const auto ticket = std::get<GameTicket>(
            client.join_game(FakeLobbyClient::DEMO_INVITE_CODE, make_character()));

    EXPECT_EQ(ticket.game_name, "Granja de Pandamos");
    EXPECT_TRUE(ticket.multiplayer);

    const auto games = saved_games();
    ASSERT_EQ(games.size(), 1u);
    EXPECT_EQ(games[0].game_id, ticket.game_id);

    connect_as("MateCapo");
    EXPECT_EQ(std::get<JoinableGameInfo>(client.find_game(FakeLobbyClient::DEMO_INVITE_CODE))
                      .player_count,
              3u);
}

TEST_F(FakeLobbyClientTest, RejectsJoiningTwice) {
    connect_as("dipa");

    EXPECT_EQ(error_of(client.find_game(FakeLobbyClient::DEMO_INVITE_CODE)),
              LobbyError::AlreadyJoined);
    EXPECT_EQ(error_of(client.join_game(FakeLobbyClient::DEMO_INVITE_CODE, make_character())),
              LobbyError::AlreadyJoined);
}

TEST_F(FakeLobbyClientTest, RejectsJoiningFullGame) {
    for (const std::string username: {"tercero", "cuarto"}) {
        connect_as(username);
        ASSERT_EQ(error_of(client.join_game(FakeLobbyClient::DEMO_INVITE_CODE, make_character())),
                  std::nullopt);
    }

    connect_as("quinto");
    EXPECT_EQ(error_of(client.find_game(FakeLobbyClient::DEMO_INVITE_CODE)), LobbyError::GameFull);
    EXPECT_EQ(error_of(client.join_game(FakeLobbyClient::DEMO_INVITE_CODE, make_character())),
              LobbyError::GameFull);
}

TEST_F(FakeLobbyClientTest, RejectsResumingForeignOrUnknownGames) {
    connect_as("dipa");
    const auto dipa_games = saved_games();

    connect_as("juana");
    for (const SavedGameInfo& game: dipa_games) {
        EXPECT_EQ(error_of(client.resume_game(game.game_id)), LobbyError::UnknownGame);
    }
    EXPECT_EQ(error_of(client.resume_game("no-existe")), LobbyError::UnknownGame);
}

TEST(FakeLobbyClientWithoutDemoGamesTest, StartsWithoutGames) {
    FakeLobbyClient client(false);
    ASSERT_EQ(client.connect("localhost", "8080", "dipa"), std::nullopt);

    EXPECT_TRUE(std::get<std::vector<SavedGameInfo>>(client.list_saved_games()).empty());
    EXPECT_EQ(std::get<LobbyError>(client.find_game(FakeLobbyClient::DEMO_INVITE_CODE)),
              LobbyError::InvalidInviteCode);
}

}
