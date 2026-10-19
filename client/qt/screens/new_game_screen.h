#ifndef CLIENT_QT_SCREENS_NEW_GAME_SCREEN_H_
#define CLIENT_QT_SCREENS_NEW_GAME_SCREEN_H_

#include <QString>
#include <QWidget>
#include <vector>

#include "lobby/lobby_types.h"

class QButtonGroup;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

namespace launcher {

// Primer paso de una partida nueva: su nombre y el mundo donde se juega.
class NewGameScreen: public QWidget {
    Q_OBJECT

private:
    std::vector<lobby::WorldInfo> worlds;
    QLineEdit* game_name_input;
    QVBoxLayout* worlds_layout;
    QButtonGroup* world_buttons;
    QPushButton* continue_button;
    QLabel* error_label;

    void update_continue_button();
    void submit();

public:
    explicit NewGameScreen(QWidget* parent = nullptr);

    void reset(const std::vector<lobby::WorldInfo>& available_worlds);
    void show_error(const QString& message);

signals:
    void back_requested();
    void continue_requested(const QString& game_name, const QString& world_id,
                            const QString& world_name);
};

}

#endif
