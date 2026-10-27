#include <QApplication>
#include <QtGlobal>

#include "gtest/gtest.h"

// Los tests de Qt del editor corren sin pantalla, salvo que QT_QPA_PLATFORM pida otra cosa.
int main(int argc, char** argv) {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
