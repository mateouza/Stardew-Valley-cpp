#ifndef CLIENT_QT_SCREENS_CHARACTER_SCREEN_H_
#define CLIENT_QT_SCREENS_CHARACTER_SCREEN_H_

#include <QString>
#include <QWidget>
#include <array>

#include "lobby/lobby_types.h"

class QLabel;
class QLineEdit;
class QPushButton;

namespace launcher {

class CharacterPreview;
class OptionSelector;

// Nombre y apariencia del personaje. La usan tanto "Nueva partida" como "Unirse a partida": el
// controlador ajusta el texto de contexto y el del botón de confirmar.
class CharacterScreen: public QWidget {
    Q_OBJECT

private:
    QLabel* context_label;
    CharacterPreview* preview;
    QLineEdit* name_input;
    OptionSelector* skin_selector;
    OptionSelector* hair_style_selector;
    OptionSelector* hair_color_selector;
    OptionSelector* shirt_selector;
    OptionSelector* pants_selector;
    QPushButton* confirm_button;
    QLabel* error_label;

    std::array<OptionSelector*, 5> selectors() const;
    lobby::CharacterAppearance current_appearance() const;
    void refresh_preview();
    void randomize_appearance();
    void update_confirm_button();
    void submit();

public:
    explicit CharacterScreen(QWidget* parent = nullptr);

    void reset_character(const QString& default_name);
    void prepare(const QString& context, const QString& confirm_text);
    void show_error(const QString& message);

signals:
    void back_requested();
    void confirm_requested(const lobby::CharacterDraft& character);
};

}

#endif
