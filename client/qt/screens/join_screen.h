#ifndef CLIENT_QT_SCREENS_JOIN_SCREEN_H_
#define CLIENT_QT_SCREENS_JOIN_SCREEN_H_

#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

namespace launcher {

// Pide el código de invitación de una partida multijugador. El código se muestra en mayúsculas
// mientras se escribe; la validación la hace el servidor.
class JoinScreen: public QWidget {
    Q_OBJECT

private:
    QLineEdit* invite_code_input;
    QPushButton* find_game_button;
    QLabel* error_label;

    void update_find_game_button();
    void submit();

public:
    explicit JoinScreen(QWidget* parent = nullptr);

    void reset();
    void show_error(const QString& message);

signals:
    void back_requested();
    void invite_code_submitted(const QString& invite_code);
};

}

#endif
