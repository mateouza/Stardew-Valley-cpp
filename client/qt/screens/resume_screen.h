#ifndef CLIENT_QT_SCREENS_RESUME_SCREEN_H_
#define CLIENT_QT_SCREENS_RESUME_SCREEN_H_

#include <QString>
#include <QWidget>
#include <vector>

#include "lobby/lobby_types.h"

class QLabel;
class QScrollArea;
class QVBoxLayout;

namespace launcher {

// Lista las partidas en las que participa el usuario, creadas por él o a las que se unió.
// Cliquear una la carga.
class ResumeScreen: public QWidget {
    Q_OBJECT

private:
    QWidget* games_container;
    QVBoxLayout* games_layout;
    QScrollArea* games_scroll;
    QLabel* intro_label;
    QLabel* empty_label;
    QLabel* error_label;

public:
    explicit ResumeScreen(QWidget* parent = nullptr);

    void set_games(const std::vector<lobby::SavedGameInfo>& games);
    void show_error(const QString& message);

signals:
    void back_requested();
    void game_selected(const QString& game_id);
};

}

#endif
