#ifndef CLIENT_QT_WIDGETS_OPTION_SELECTOR_H_
#define CLIENT_QT_WIDGETS_OPTION_SELECTOR_H_

#include <QColor>
#include <QString>
#include <QWidget>
#include <optional>
#include <vector>

class QLabel;

namespace launcher {

// Selector "◀ valor ▶" que recorre las opciones en forma circular.
class OptionSelector: public QWidget {
    Q_OBJECT

public:
    struct Option {
        QString name;
        std::optional<QColor> swatch;
    };

private:
    std::vector<Option> options;
    int current = 0;
    QLabel* swatch_label;
    QLabel* value_label;

    void step(int delta);
    void refresh();

public:
    OptionSelector(const QString& caption, std::vector<Option> options, QWidget* parent = nullptr);

    int current_index() const;
    int option_count() const;
    void set_current_index(int index);

signals:
    void current_index_changed(int index);
};

}

#endif
