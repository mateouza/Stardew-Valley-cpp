#include "world_card.h"

#include <QLabel>
#include <QString>
#include <QVBoxLayout>

#include "ui_helpers.h"

namespace launcher {

WorldCard::WorldCard(const lobby::WorldInfo& world, QWidget* parent): QPushButton(parent) {
    setObjectName(QString::fromStdString("world_" + world.id));
    setProperty("variant", "card");
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFixedHeight(76);

    QLabel* name = create_label(QString::fromStdString(world.name), "card_title", this);
    QLabel* description =
            create_label(QString::fromStdString(world.description), "card_detail", this);
    description->setWordWrap(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 10, 16, 10);
    layout->setSpacing(2);
    for (QLabel* label: {name, description}) {
        label->setAttribute(Qt::WA_TransparentForMouseEvents);
        layout->addWidget(label);
    }
}

}
