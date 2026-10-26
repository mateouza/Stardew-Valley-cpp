#include "character_screen.h"

#include <QColor>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "qt/widgets/character_options.h"
#include "qt/widgets/character_preview.h"
#include "qt/widgets/option_selector.h"
#include "qt/widgets/ui_helpers.h"

namespace launcher {

namespace {

template <std::size_t N>
std::vector<OptionSelector::Option> color_options(const std::array<ColorOption, N>& colors) {
    std::vector<OptionSelector::Option> options;
    for (const ColorOption& color: colors) {
        options.push_back({QString::fromUtf8(color.name), QColor(color.rgb)});
    }
    return options;
}

std::vector<OptionSelector::Option> hair_style_options() {
    std::vector<OptionSelector::Option> options;
    for (const char* name: HAIR_STYLES) {
        options.push_back({QString::fromUtf8(name), std::nullopt});
    }
    return options;
}

}

CharacterScreen::CharacterScreen(QWidget* parent):
        QWidget(parent),
        context_label(create_hint("", this)),
        preview(new CharacterPreview(this)),
        name_input(new QLineEdit(this)),
        skin_selector(new OptionSelector("Piel", color_options(SKIN_TONES), this)),
        hair_style_selector(new OptionSelector("Peinado", hair_style_options(), this)),
        hair_color_selector(new OptionSelector("Pelo", color_options(HAIR_COLORS), this)),
        shirt_selector(new OptionSelector("Remera", color_options(SHIRT_COLORS), this)),
        pants_selector(new OptionSelector("Pantalón", color_options(PANTS_COLORS), this)),
        confirm_button(create_button("", "primary", this)),
        error_label(create_error_label(this)) {
    setObjectName("character_screen");
    setFocusProxy(name_input);

    context_label->setObjectName("context_label");
    context_label->setAlignment(Qt::AlignCenter);
    preview->setObjectName("character_preview");
    preview->setFixedSize(180, 252);
    name_input->setObjectName("character_name_input");
    name_input->setPlaceholderText("Ej: Juanceto01");
    name_input->setMaxLength(static_cast<int>(lobby::MAX_CHARACTER_NAME_LENGTH));
    skin_selector->setObjectName("skin_selector");
    hair_style_selector->setObjectName("hair_style_selector");
    hair_color_selector->setObjectName("hair_color_selector");
    shirt_selector->setObjectName("shirt_selector");
    pants_selector->setObjectName("pants_selector");
    confirm_button->setObjectName("confirm_button");

    QPushButton* random_button = create_button("Aleatorio", "secondary", this);
    random_button->setObjectName("random_button");
    QPushButton* back_button = create_button("Volver", "secondary", this);
    back_button->setObjectName("back_button");
    back_button->setShortcut(QKeySequence(Qt::Key_Escape));

    auto* preview_column = new QVBoxLayout();
    preview_column->addWidget(preview);
    preview_column->addWidget(random_button);
    preview_column->addStretch();

    auto* form_column = new QVBoxLayout();
    form_column->setSpacing(8);
    add_field(form_column, "Nombre del personaje", name_input);
    form_column->addSpacing(6);
    for (OptionSelector* selector: selectors()) {
        form_column->addWidget(selector);
    }
    form_column->addStretch();

    auto* editor_row = new QHBoxLayout();
    editor_row->setSpacing(24);
    editor_row->addLayout(preview_column);
    editor_row->addLayout(form_column, 1);

    Panel panel = create_panel(720, this);
    panel.content->addWidget(context_label);
    panel.content->addSpacing(4);
    panel.content->addLayout(editor_row);
    panel.content->addWidget(error_label);
    panel.content->addSpacing(8);
    panel.content->addLayout(create_button_row(back_button, confirm_button));
    layout_screen(this, create_screen_title("Tu personaje", this), panel.frame);

    for (OptionSelector* selector: selectors()) {
        connect(selector, &OptionSelector::current_index_changed, this,
                &CharacterScreen::refresh_preview);
    }
    connect(name_input, &QLineEdit::textChanged, this, &CharacterScreen::update_confirm_button);
    connect(name_input, &QLineEdit::returnPressed, this, &CharacterScreen::submit);
    connect(random_button, &QPushButton::clicked, this, &CharacterScreen::randomize_appearance);
    connect(back_button, &QPushButton::clicked, this, &CharacterScreen::back_requested);
    connect(confirm_button, &QPushButton::clicked, this, &CharacterScreen::submit);
    update_confirm_button();
    refresh_preview();
}

std::array<OptionSelector*, 5> CharacterScreen::selectors() const {
    return {skin_selector, hair_style_selector, hair_color_selector, shirt_selector,
            pants_selector};
}

lobby::CharacterAppearance CharacterScreen::current_appearance() const {
    auto index = [](const OptionSelector* selector) {
        return static_cast<std::uint8_t>(selector->current_index());
    };
    return lobby::CharacterAppearance{index(skin_selector), index(hair_style_selector),
                                      index(hair_color_selector), index(shirt_selector),
                                      index(pants_selector)};
}

void CharacterScreen::refresh_preview() { preview->set_appearance(current_appearance()); }

void CharacterScreen::randomize_appearance() {
    for (OptionSelector* selector: selectors()) {
        selector->set_current_index(
                static_cast<int>(QRandomGenerator::global()->bounded(selector->option_count())));
    }
}

void CharacterScreen::update_confirm_button() {
    confirm_button->setEnabled(!name_input->text().trimmed().isEmpty());
}

void CharacterScreen::reset_character(const QString& default_name) {
    name_input->setText(default_name.left(static_cast<int>(lobby::MAX_CHARACTER_NAME_LENGTH)));
    randomize_appearance();
    set_error(error_label, "");
}

void CharacterScreen::prepare(const QString& context, const QString& confirm_text) {
    context_label->setText(context);
    confirm_button->setText(confirm_text);
    set_error(error_label, "");
}

void CharacterScreen::show_error(const QString& message) { set_error(error_label, message); }

void CharacterScreen::submit() {
    if (!confirm_button->isEnabled()) {
        return;
    }

    set_error(error_label, "");
    emit confirm_requested(lobby::CharacterDraft{name_input->text().trimmed().toStdString(),
                                                 current_appearance()});
}

}
