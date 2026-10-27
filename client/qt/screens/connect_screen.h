#ifndef CLIENT_QT_SCREENS_CONNECT_SCREEN_H_
#define CLIENT_QT_SCREENS_CONNECT_SCREEN_H_

#include <QString>
#include <QWidget>

#include "qt/launcher.h"

class QLabel;
class QLineEdit;
class QPushButton;

namespace launcher {

class ConnectScreen: public QWidget {
    Q_OBJECT

private:
    QLineEdit* username_input;
    QLineEdit* host_input;
    QLineEdit* port_input;
    QPushButton* connect_button;
    QLabel* error_label;

    void update_connect_button();
    void submit();

public:
    explicit ConnectScreen(const LauncherOptions& options, QWidget* parent = nullptr);

    void show_error(const QString& message);

signals:
    void connect_requested(const QString& username, const QString& host, const QString& port);
    void quit_requested();
};

}

#endif
