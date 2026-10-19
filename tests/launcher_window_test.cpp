#include "qt/launcher_window.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QtGlobal>
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

    void connect_as(const QString& username) {
        widget<QLineEdit>("connect_screen", "username_input")->setText(username);
        widget<QPushButton>("connect_screen", "connect_button")->click();
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

TEST_F(LauncherWindowTest, ShowsServerErrorAndStaysOnConnectScreen) {
    widget<QLineEdit>("connect_screen", "port_input")->setText("puerto");
    connect_as("juana");

    EXPECT_EQ(current_screen(), "connect_screen");
    EXPECT_TRUE(shows_error("connect_screen"));
}

TEST_F(LauncherWindowTest, ShowsUsernameErrorFromServer) {
    connect_as("con espacio");

    EXPECT_EQ(current_screen(), "connect_screen");
    EXPECT_TRUE(shows_error("connect_screen"));
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
