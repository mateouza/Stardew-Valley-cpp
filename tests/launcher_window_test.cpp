#include "qt/launcher_window.h"

#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QTest>
#include <QtGlobal>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>

#include "gtest/gtest.h"
#include "lobby/fake_lobby_client.h"

void PrintTo(const QString& text, std::ostream* os) { *os << '"' << text.toStdString() << '"'; }

namespace launcher {

class LauncherWindowTest: public ::testing::Test {
protected:
    lobby::FakeLobbyClient lobby_client;
    LauncherWindow window{lobby_client, LauncherOptions{}};

    template <typename T>
    T* widget(const char* screen_name, const char* widget_name) {
        auto* screen = window.findChild<QWidget*>(screen_name);
        T* found = screen ? screen->findChild<T*>(widget_name) : nullptr;
        if (!found) {
            throw std::runtime_error(std::string("No existe ") + screen_name + "/" + widget_name);
        }
        return found;
    }

    QString current_screen() {
        return window.findChild<QStackedWidget*>()->currentWidget()->objectName();
    }

    bool shows_error(const char* screen_name) {
        auto* error_label = widget<QLabel>(screen_name, "error_label");
        return error_label->isVisibleTo(&window) && !error_label->text().isEmpty();
    }

    void click(const char* screen_name, const char* button_name) {
        widget<QPushButton>(screen_name, button_name)->click();
    }

    void type(const char* screen_name, const char* input_name, const QString& text) {
        widget<QLineEdit>(screen_name, input_name)->setText(text);
    }

    void connect_as(const QString& username) {
        type("connect_screen", "username_input", username);
        click("connect_screen", "connect_button");
    }

    void open_new_game_as(const QString& username) {
        connect_as(username);
        click("main_menu_screen", "new_game_button");
    }

    void look_up_code_as(const QString& username, const QString& invite_code) {
        connect_as(username);
        click("main_menu_screen", "join_button");
        type("join_screen", "invite_code_input", invite_code);
        click("join_screen", "find_game_button");
    }
};

TEST_F(LauncherWindowTest, StartsOnConnectScreenWithoutTicket) {
    EXPECT_EQ(current_screen(), "connect_screen");
    EXPECT_FALSE(shows_error("connect_screen"));
    EXPECT_FALSE(window.get_ticket().has_value());
}

TEST_F(LauncherWindowTest, EnablesConnectOnlyWithUsername) {
    auto* connect_button = widget<QPushButton>("connect_screen", "connect_button");
    EXPECT_FALSE(connect_button->isEnabled());

    widget<QLineEdit>("connect_screen", "username_input")->setText("juana");
    EXPECT_TRUE(connect_button->isEnabled());
}

TEST_F(LauncherWindowTest, EnablesConnectOnlyWithPortInRange) {
    type("connect_screen", "username_input", "juana");
    auto* connect_button = widget<QPushButton>("connect_screen", "connect_button");

    for (const char* port: {"0", "65536", "99999", "abc", ""}) {
        type("connect_screen", "port_input", port);
        EXPECT_FALSE(connect_button->isEnabled()) << port;
    }
    for (const char* port: {"1", "65535"}) {
        type("connect_screen", "port_input", port);
        EXPECT_TRUE(connect_button->isEnabled()) << port;
    }
}

TEST_F(LauncherWindowTest, TypingInPortKeepsOnlyDigits) {
    window.show();
    auto* port_input = widget<QLineEdit>("connect_screen", "port_input");
    port_input->clear();

    QTest::keyClicks(port_input, "8a0-8 0");

    EXPECT_EQ(port_input->text(), "8080");
}

TEST_F(LauncherWindowTest, EnterWithPortOutOfRangeDoesNotConnect) {
    window.show();
    type("connect_screen", "username_input", "juana");
    type("connect_screen", "port_input", "0");

    QTest::keyClick(widget<QLineEdit>("connect_screen", "port_input"), Qt::Key_Return);

    EXPECT_EQ(current_screen(), "connect_screen");
    EXPECT_FALSE(shows_error("connect_screen"));
}

TEST_F(LauncherWindowTest, EnterWithValidPortConnects) {
    window.show();
    type("connect_screen", "username_input", "juana");
    type("connect_screen", "port_input", "65535");

    QTest::keyClick(widget<QLineEdit>("connect_screen", "port_input"), Qt::Key_Return);

    EXPECT_EQ(current_screen(), "main_menu_screen");
}

TEST_F(LauncherWindowTest, ConnectingShowsMainMenuWithSession) {
    connect_as("juana");

    EXPECT_EQ(current_screen(), "main_menu_screen");
    EXPECT_EQ(widget<QLabel>("main_menu_screen", "session_label")->text(),
              "Usuario: juana · Servidor: localhost:8080");
}

TEST_F(LauncherWindowTest, QuitClosesWithoutTicket) {
    window.show();
    connect_as("juana");

    widget<QPushButton>("main_menu_screen", "quit_button")->click();

    EXPECT_FALSE(window.isVisible());
    EXPECT_FALSE(window.get_ticket().has_value());
}

TEST_F(LauncherWindowTest, NewGameListsWorldsWithFirstOneSelected) {
    open_new_game_as("juana");

    EXPECT_EQ(current_screen(), "new_game_screen");
    EXPECT_TRUE(widget<QPushButton>("new_game_screen", "world_valle_clasico")->isChecked());
    EXPECT_FALSE(widget<QPushButton>("new_game_screen", "world_orilla_del_rio")->isChecked());
    EXPECT_FALSE(widget<QPushButton>("new_game_screen", "world_colinas_altas")->isChecked());
}

TEST_F(LauncherWindowTest, ContinueRequiresGameName) {
    open_new_game_as("juana");
    auto* continue_button = widget<QPushButton>("new_game_screen", "continue_button");

    EXPECT_FALSE(continue_button->isEnabled());
    type("new_game_screen", "game_name_input", "   ");
    EXPECT_FALSE(continue_button->isEnabled());
    type("new_game_screen", "game_name_input", "Granja Girasol");
    EXPECT_TRUE(continue_button->isEnabled());
}

TEST_F(LauncherWindowTest, CreatingGameFinishesWithSinglePlayerTicket) {
    window.show();
    open_new_game_as("juana");
    type("new_game_screen", "game_name_input", "Granja Girasol");
    click("new_game_screen", "world_orilla_del_rio");
    click("new_game_screen", "continue_button");

    ASSERT_EQ(current_screen(), "character_screen");
    EXPECT_EQ(widget<QLineEdit>("character_screen", "character_name_input")->text(), "juana");
    EXPECT_EQ(widget<QLabel>("character_screen", "context_label")->text(),
              "Partida «Granja Girasol» · Mundo Orilla del Río");
    EXPECT_EQ(widget<QPushButton>("character_screen", "confirm_button")->text(), "Crear partida");

    type("character_screen", "character_name_input", "Juani");
    click("character_screen", "confirm_button");

    ASSERT_TRUE(window.get_ticket().has_value());
    EXPECT_EQ(window.get_ticket()->game_name, "Granja Girasol");
    EXPECT_EQ(window.get_ticket()->character.name, "Juani");
    EXPECT_FALSE(window.get_ticket()->multiplayer);
    EXPECT_FALSE(window.isVisible());
}

TEST_F(LauncherWindowTest, ConfirmRequiresCharacterName) {
    open_new_game_as("juana");
    type("new_game_screen", "game_name_input", "Granja");
    click("new_game_screen", "continue_button");

    type("character_screen", "character_name_input", "  ");
    EXPECT_FALSE(widget<QPushButton>("character_screen", "confirm_button")->isEnabled());
}

TEST_F(LauncherWindowTest, GoingBackKeepsWhatWasEntered) {
    open_new_game_as("juana");
    type("new_game_screen", "game_name_input", "Granja Girasol");
    click("new_game_screen", "continue_button");
    type("character_screen", "character_name_input", "Juani");

    click("character_screen", "back_button");
    EXPECT_EQ(current_screen(), "new_game_screen");
    EXPECT_EQ(widget<QLineEdit>("new_game_screen", "game_name_input")->text(), "Granja Girasol");

    click("new_game_screen", "continue_button");
    EXPECT_EQ(widget<QLineEdit>("character_screen", "character_name_input")->text(), "Juani");

    click("character_screen", "back_button");
    click("new_game_screen", "back_button");
    EXPECT_EQ(current_screen(), "main_menu_screen");
}

TEST_F(LauncherWindowTest, CharacterNameAcceptsOnlyListedCharacters) {
    window.show();
    open_new_game_as("juana");
    type("new_game_screen", "game_name_input", "Granja");
    click("new_game_screen", "continue_button");
    auto* name_input = widget<QLineEdit>("character_screen", "character_name_input");
    name_input->clear();

    // keyClicks solo simula teclas ASCII; así se manda cada tecla con su texto, como un teclado
    // en español.
    for (const QChar c: QString::fromUtf8("Peñón_")) {
        QTest::sendKeyEvent(QTest::Click, name_input, Qt::Key_unknown, QString(c), Qt::NoModifier);
    }
    name_input->insert(QString::fromUtf8("🙂"));

    EXPECT_EQ(name_input->text(), QString::fromUtf8("Peñón"));
    EXPECT_TRUE(widget<QPushButton>("character_screen", "confirm_button")->isEnabled());
}

TEST_F(LauncherWindowTest, PastedGameNameFollowsTheSameRules) {
    open_new_game_as("juana");
    auto* game_name_input = widget<QLineEdit>("new_game_screen", "game_name_input");

    QApplication::clipboard()->setText(QString::fromUtf8("Granja 🙂"));
    game_name_input->paste();
    EXPECT_TRUE(game_name_input->text().isEmpty());

    QApplication::clipboard()->setText(QString::fromUtf8("Huerta de Peñón"));
    game_name_input->paste();
    EXPECT_EQ(game_name_input->text(), QString::fromUtf8("Huerta de Peñón"));
}

TEST_F(LauncherWindowTest, LongGameNameIsCutAtTheLimit) {
    open_new_game_as("juana");
    auto* game_name_input = widget<QLineEdit>("new_game_screen", "game_name_input");

    game_name_input->insert(
            QString(static_cast<int>(lobby::MAX_GAME_NAME_LENGTH) + 1, QChar(0xF1)));

    EXPECT_EQ(game_name_input->text().size(), static_cast<qsizetype>(lobby::MAX_GAME_NAME_LENGTH));
    EXPECT_TRUE(widget<QPushButton>("new_game_screen", "continue_button")->isEnabled());
}

TEST_F(LauncherWindowTest, UsernameWithUnderscoreIsNotCopiedAsCharacterName) {
    open_new_game_as("Mate_Capo");
    type("new_game_screen", "game_name_input", "Granja");
    click("new_game_screen", "continue_button");

    EXPECT_TRUE(widget<QLineEdit>("character_screen", "character_name_input")->text().isEmpty());
    EXPECT_FALSE(widget<QPushButton>("character_screen", "confirm_button")->isEnabled());
}

TEST_F(LauncherWindowTest, JoiningWithInviteCodeFinishesWithMultiplayerTicket) {
    look_up_code_as("juana", "valle1");

    ASSERT_EQ(current_screen(), "character_screen");
    EXPECT_EQ(widget<QLabel>("character_screen", "context_label")->text(),
              "Te unís a «Granja de Pandamos», de Pandamos · 2/4 jugadores");
    EXPECT_EQ(widget<QPushButton>("character_screen", "confirm_button")->text(),
              "Unirme a la partida");

    click("character_screen", "confirm_button");

    ASSERT_TRUE(window.get_ticket().has_value());
    EXPECT_EQ(window.get_ticket()->game_name, "Granja de Pandamos");
    EXPECT_EQ(window.get_ticket()->character.name, "juana");
    EXPECT_TRUE(window.get_ticket()->multiplayer);
}

TEST_F(LauncherWindowTest, UnknownInviteCodeShowsError) {
    look_up_code_as("juana", "NOEXISTE");

    EXPECT_EQ(current_screen(), "join_screen");
    EXPECT_TRUE(shows_error("join_screen"));
}

TEST_F(LauncherWindowTest, JoiningOwnGameShowsError) {
    look_up_code_as("dipa", "VALLE1");

    EXPECT_EQ(current_screen(), "join_screen");
    EXPECT_EQ(widget<QLabel>("join_screen", "error_label")->text(),
              "Ya formás parte de esa partida. Entrá desde «Retomar partida».");
}

TEST_F(LauncherWindowTest, InviteCodeIsUppercasedWhileTyping) {
    connect_as("juana");
    click("main_menu_screen", "join_button");

    widget<QLineEdit>("join_screen", "invite_code_input")->insert("valle1");

    EXPECT_EQ(widget<QLineEdit>("join_screen", "invite_code_input")->text(), "VALLE1");
}

TEST_F(LauncherWindowTest, BackFromCharacterReturnsToJoinScreen) {
    look_up_code_as("juana", "VALLE1");

    click("character_screen", "back_button");

    EXPECT_EQ(current_screen(), "join_screen");
    EXPECT_EQ(widget<QLineEdit>("join_screen", "invite_code_input")->text(), "VALLE1");
}

TEST_F(LauncherWindowTest, ResumeListsGamesWithMultiplayerBadge) {
    connect_as("dipa");
    click("main_menu_screen", "resume_button");

    ASSERT_EQ(current_screen(), "resume_screen");
    auto* shared_farm = widget<QPushButton>("resume_screen", "saved_game_partida-1");
    auto* solo_farm = widget<QPushButton>("resume_screen", "saved_game_partida-2");
    EXPECT_EQ(shared_farm->findChild<QLabel*>("badge")->text(), "Multijugador");
    EXPECT_EQ(solo_farm->findChild<QLabel*>("badge")->text(), "Un jugador");
    EXPECT_FALSE(widget<QLabel>("resume_screen", "empty_label")->isVisibleTo(&window));
}

TEST_F(LauncherWindowTest, SelectingSavedGameFinishesWithTicket) {
    connect_as("dipa");
    click("main_menu_screen", "resume_button");

    click("resume_screen", "saved_game_partida-2");

    ASSERT_TRUE(window.get_ticket().has_value());
    EXPECT_EQ(window.get_ticket()->game_name, "Huerta de Dipa");
    EXPECT_EQ(window.get_ticket()->character.name, "Dipa");
    EXPECT_FALSE(window.get_ticket()->multiplayer);
}

TEST_F(LauncherWindowTest, UserWithoutGamesSeesEmptyMessage) {
    connect_as("juana");
    click("main_menu_screen", "resume_button");

    EXPECT_EQ(current_screen(), "resume_screen");
    EXPECT_TRUE(widget<QLabel>("resume_screen", "empty_label")->isVisibleTo(&window));
    EXPECT_FALSE(widget<QLabel>("resume_screen", "intro_label")->isVisibleTo(&window));
    EXPECT_FALSE(widget<QWidget>("resume_screen", "games_scroll")->isVisibleTo(&window));

    click("resume_screen", "back_button");
    EXPECT_EQ(current_screen(), "main_menu_screen");
}

class ConnectRejectingLobbyClient: public lobby::FakeLobbyClient {
private:
    lobby::LobbyError error;

public:
    explicit ConnectRejectingLobbyClient(lobby::LobbyError error): error(error) {}

    std::optional<lobby::LobbyError> connect(const std::string&, const std::string&,
                                             const std::string&) override {
        return error;
    }
};

QLabel* connect_error_label(const LauncherWindow& window) {
    return window.findChild<QWidget*>("connect_screen")->findChild<QLabel*>("error_label");
}

TEST(LauncherWindowErrorsTest, ServerUnavailableKeepsConnectScreenOpen) {
    ConnectRejectingLobbyClient lobby_client(lobby::LobbyError::ServerUnavailable);
    LauncherWindow window(lobby_client, LauncherOptions{"localhost", "8080", "juana"});
    window.findChild<QPushButton*>("connect_button")->click();

    EXPECT_EQ(window.findChild<QStackedWidget*>()->currentWidget()->objectName(), "connect_screen");
    EXPECT_TRUE(connect_error_label(window)->isVisibleTo(&window));
    EXPECT_EQ(connect_error_label(window)->text(),
              "No se pudo conectar con el servidor. Revisá la dirección y el puerto.");
}

TEST(LauncherWindowErrorsTest, UsernameRejectedByServerShowsItsMessage) {
    ConnectRejectingLobbyClient lobby_client(lobby::LobbyError::InvalidUsername);
    LauncherWindow window(lobby_client, LauncherOptions{"localhost", "8080", "juana"});
    window.findChild<QPushButton*>("connect_button")->click();

    EXPECT_EQ(window.findChild<QStackedWidget*>()->currentWidget()->objectName(), "connect_screen");
    EXPECT_TRUE(connect_error_label(window)->text().startsWith("El nombre de usuario"));
}

class RejectingLobbyClient: public lobby::FakeLobbyClient {
public:
    lobby::LobbyResult<lobby::GameTicket> create_game(const std::string&, const std::string&,
                                                      const lobby::CharacterDraft&) override {
        return lobby::LobbyError::UnknownWorld;
    }
};

TEST(LauncherWindowErrorsTest, ServerRejectionKeepsCharacterScreenOpen) {
    RejectingLobbyClient lobby_client;
    LauncherWindow window(lobby_client, LauncherOptions{"localhost", "8080", "juana"});
    window.findChild<QPushButton*>("connect_button")->click();
    window.findChild<QPushButton*>("new_game_button")->click();
    window.findChild<QLineEdit*>("game_name_input")->setText("Granja");
    window.findChild<QPushButton*>("continue_button")->click();
    window.findChild<QPushButton*>("confirm_button")->click();

    auto* character_screen = window.findChild<QWidget*>("character_screen");
    auto* error_label = character_screen->findChild<QLabel*>("error_label");
    EXPECT_EQ(window.findChild<QStackedWidget*>()->currentWidget(), character_screen);
    EXPECT_TRUE(error_label->isVisibleTo(&window));
    EXPECT_EQ(error_label->text(), "El mundo elegido ya no está disponible.");
    EXPECT_FALSE(window.get_ticket().has_value());
}

TEST(LauncherWindowOptionsTest, PrefillsConnectionFields) {
    lobby::FakeLobbyClient lobby_client;
    LauncherWindow window(lobby_client, LauncherOptions{"10.0.0.5", "9000", "dipa"});

    EXPECT_EQ(window.findChild<QLineEdit*>("host_input")->text(), "10.0.0.5");
    EXPECT_EQ(window.findChild<QLineEdit*>("port_input")->text(), "9000");
    EXPECT_EQ(window.findChild<QLineEdit*>("username_input")->text(), "dipa");
}

}

int main(int argc, char** argv) {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
