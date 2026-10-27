#include <QApplication>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QRect>
#include <QString>
#include <QtGlobal>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "gtest/gtest.h"
#include "lobby/lobby_types.h"
#include "qt/widgets/character_sprite.h"
#include "qt/widgets/option_selector.h"

void PrintTo(const QString& text, std::ostream* os) { *os << '"' << text.toStdString() << '"'; }

namespace launcher {

namespace {

bool is_opaque(const QImage& image, int x, int y) { return qAlpha(image.pixel(x, y)) > 0; }

}

TEST(CharacterSpriteTest, RendersSpriteWithOutlineMargin) {
    const QImage sprite = render_character(lobby::CharacterAppearance{});

    EXPECT_EQ(sprite.width(), 18);
    EXPECT_EQ(sprite.height(), 26);
    EXPECT_FALSE(is_opaque(sprite, 0, 0));
    EXPECT_TRUE(is_opaque(sprite, 4, 23));  // zapato izquierdo
    EXPECT_TRUE(is_opaque(sprite, 3, 23));  // contorno del zapato
    EXPECT_FALSE(is_opaque(sprite, 2, 23));
}

TEST(CharacterSpriteTest, ColorsDependOnAppearance) {
    const QImage green_shirt = render_character(lobby::CharacterAppearance{0, 0, 0, 0, 0});
    const QImage blue_shirt = render_character(lobby::CharacterAppearance{0, 0, 0, 1, 0});

    EXPECT_NE(green_shirt, blue_shirt);
    EXPECT_EQ(green_shirt.pixel(5, 14), qRgb(88, 160, 72));
}

TEST(CharacterSpriteTest, HairStylesChangeSilhouette) {
    const QImage short_hair = render_character(lobby::CharacterAppearance{0, 0, 0, 0, 0});
    const QImage bun = render_character(lobby::CharacterAppearance{0, 3, 0, 0, 0});

    EXPECT_FALSE(is_opaque(short_hair, 8, 1));
    EXPECT_TRUE(is_opaque(bun, 8, 1));
}

TEST(CharacterSpriteTest, OutOfRangeIndicesWrapAround) {
    const QImage sprite = render_character(lobby::CharacterAppearance{255, 255, 255, 255, 255});

    EXPECT_EQ(sprite.size(), QSize(18, 26));
}

TEST(CharacterSpriteTest, FitsWithIntegerScaleCentered) {
    EXPECT_EQ(fit_character(QRect(0, 0, 100, 100)), QRect(23, 11, 54, 78));
    EXPECT_EQ(fit_character(QRect(10, 20, 180, 260)), QRect(10, 20, 180, 260));
}

TEST(CharacterSpriteTest, NeverScalesBelowOriginalSize) {
    EXPECT_EQ(fit_character(QRect(0, 0, 10, 10)).size(), QSize(18, 26));
}

class OptionSelectorTest: public ::testing::Test {
protected:
    OptionSelector selector{"Remera",
                            std::vector<OptionSelector::Option>{{"Verde", QColor(Qt::green)},
                                                                {"Azul", QColor(Qt::blue)},
                                                                {"Rojo", QColor(Qt::red)}}};

    void click(const char* button_name) { selector.findChild<QPushButton*>(button_name)->click(); }

    QString value() { return selector.findChild<QLabel*>("value_label")->text(); }
};

TEST_F(OptionSelectorTest, StartsOnFirstOption) {
    EXPECT_EQ(selector.current_index(), 0);
    EXPECT_EQ(selector.option_count(), 3);
    EXPECT_EQ(value(), "Verde");
}

TEST_F(OptionSelectorTest, WrapsAroundInBothDirections) {
    click("previous_button");
    EXPECT_EQ(selector.current_index(), 2);
    EXPECT_EQ(value(), "Rojo");

    click("next_button");
    EXPECT_EQ(selector.current_index(), 0);
}

TEST_F(OptionSelectorTest, NotifiesOnlyRealChanges) {
    std::vector<int> notified;
    QObject::connect(&selector, &OptionSelector::current_index_changed,
                     [&notified](int index) { notified.push_back(index); });

    selector.set_current_index(1);
    selector.set_current_index(1);
    selector.set_current_index(7);
    selector.set_current_index(-1);

    EXPECT_EQ(notified, std::vector<int>{1});
    EXPECT_EQ(value(), "Azul");
}

TEST(OptionSelectorWithoutOptionsTest, RejectsEmptyOptions) {
    EXPECT_THROW(OptionSelector("Vacío", {}), std::invalid_argument);
}

}

int main(int argc, char** argv) {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
