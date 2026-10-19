#include "launcher_style.h"

namespace launcher {

namespace {

constexpr const char STYLESHEET[] = R"(
QWidget {
    font-family: "DejaVu Sans";
    font-size: 15px;
    color: #3b2416;
}

QLabel[role="logo_title"] {
    font-size: 56px;
    font-weight: bold;
    color: #fff4d6;
}

QLabel[role="logo_subtitle"] {
    font-size: 17px;
    font-weight: bold;
    color: #fff4d6;
}

QFrame[role="panel"] {
    background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #fbefd5, stop: 1 #f1d9ab);
    border: 4px solid #5b3a1e;
    border-radius: 12px;
}

QLabel[role="panel_title"] {
    font-size: 22px;
    font-weight: bold;
    color: #5b3a1e;
}

QLabel[role="field_label"] {
    font-weight: bold;
    color: #5b3a1e;
}

QLabel[role="hint"], QLabel[role="session"] {
    font-size: 13px;
    color: #7a5a3a;
}

QLabel[role="error"] {
    font-weight: bold;
    color: #8f1d14;
    background-color: #f8d3c5;
    border: 2px solid #c8553d;
    border-radius: 6px;
    padding: 6px 10px;
}

QPushButton {
    font-size: 16px;
    font-weight: bold;
    color: #fff4d6;
    background-color: #c98a4b;
    border: 3px solid #5b3a1e;
    border-bottom-width: 5px;
    border-radius: 8px;
    padding: 8px 22px;
}

QPushButton:hover {
    background-color: #dda15e;
}

QPushButton:pressed {
    background-color: #a86f39;
    border-bottom-width: 3px;
    padding-top: 10px;
}

QPushButton:disabled {
    color: #f3ead9;
    background-color: #c9b79d;
    border-color: #9a8467;
}

QPushButton[variant="secondary"] {
    color: #5b3a1e;
    background-color: #ead3a8;
}

QPushButton[variant="secondary"]:hover {
    background-color: #f3e0bb;
}

QPushButton[variant="secondary"]:pressed {
    background-color: #dcc08e;
}

QPushButton[variant="arrow"] {
    font-size: 13px;
    padding: 0px;
    border-bottom-width: 4px;
    border-radius: 6px;
}

QPushButton[variant="arrow"]:pressed {
    padding-top: 1px;
    border-bottom-width: 3px;
}

QLabel[role="option_value"] {
    font-weight: bold;
    background-color: #fffaf0;
    border: 2px solid #c99a66;
    border-radius: 6px;
    padding: 4px 6px;
}

QLineEdit {
    font-size: 16px;
    background-color: #fffaf0;
    border: 3px solid #c99a66;
    border-radius: 8px;
    padding: 6px 10px;
    selection-background-color: #dda15e;
}

QLineEdit:focus {
    border-color: #5b3a1e;
}
)";

}

QString launcher_stylesheet() { return QString::fromUtf8(STYLESHEET); }

}
