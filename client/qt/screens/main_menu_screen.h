#ifndef CLIENT_QT_SCREENS_MAIN_MENU_SCREEN_H_
#define CLIENT_QT_SCREENS_MAIN_MENU_SCREEN_H_

#include <QString>
#include <QWidget>

class QLabel;

namespace launcher {

class MainMenuScreen: public QWidget {
    Q_OBJECT

private:
    QLabel* session_label;
    QLabel* error_label;

public:
    explicit MainMenuScreen(QWidget* parent = nullptr);

    void set_session(const QString& username, const QString& server);
    void show_error(const QString& message);

signals:
    void new_game_requested();
    void join_requested();
    void resume_requested();
    void quit_requested();
};

}

#endif
