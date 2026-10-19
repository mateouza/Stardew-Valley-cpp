#include "option_selector.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <stdexcept>
#include <utility>

#include "ui_helpers.h"

namespace launcher {

OptionSelector::OptionSelector(const QString& caption, std::vector<Option> options,
                               QWidget* parent):
        QWidget(parent),
        options(std::move(options)),
        swatch_label(new QLabel(this)),
        value_label(create_label("", "option_value", this)) {
    if (this->options.empty()) {
        throw std::invalid_argument("Un selector necesita al menos una opción");
    }

    QLabel* caption_label = create_label(caption, "field_label", this);
    caption_label->setFixedWidth(130);

    QPushButton* previous_button = create_button("◀", "arrow", this);
    QPushButton* next_button = create_button("▶", "arrow", this);
    previous_button->setObjectName("previous_button");
    next_button->setObjectName("next_button");
    for (QPushButton* button: {previous_button, next_button}) {
        button->setFixedSize(36, 32);
        button->setFocusPolicy(Qt::NoFocus);
    }

    swatch_label->setFixedSize(18, 18);
    QSizePolicy swatch_policy = swatch_label->sizePolicy();
    swatch_policy.setRetainSizeWhenHidden(true);
    swatch_label->setSizePolicy(swatch_policy);

    value_label->setObjectName("value_label");
    value_label->setAlignment(Qt::AlignCenter);
    value_label->setMinimumWidth(110);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(caption_label);
    layout->addWidget(previous_button);
    layout->addWidget(swatch_label);
    layout->addWidget(value_label, 1);
    layout->addWidget(next_button);

    connect(previous_button, &QPushButton::clicked, this, [this] { step(-1); });
    connect(next_button, &QPushButton::clicked, this, [this] { step(1); });
    refresh();
}

int OptionSelector::current_index() const { return current; }

int OptionSelector::option_count() const { return static_cast<int>(options.size()); }

void OptionSelector::set_current_index(int index) {
    if (index < 0 || index >= option_count() || index == current) {
        return;
    }

    current = index;
    refresh();
    emit current_index_changed(current);
}

void OptionSelector::step(int delta) {
    set_current_index((current + delta + option_count()) % option_count());
}

void OptionSelector::refresh() {
    const Option& option = options[current];
    value_label->setText(option.name);
    swatch_label->setVisible(option.swatch.has_value());
    if (option.swatch) {
        swatch_label->setStyleSheet(
                QString("background-color: %1; border: 2px solid #5b3a1e; border-radius: 4px;")
                        .arg(option.swatch->name()));
    }
}

}
