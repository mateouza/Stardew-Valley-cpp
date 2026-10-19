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

QLabel[role="screen_title"] {
    font-size: 40px;
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

QLineEdit[role="invite_code"] {
    font-family: "DejaVu Sans Mono";
    font-size: 28px;
    font-weight: bold;
    padding: 8px 10px;
}

QPushButton[variant="card"] {
    color: #3b2416;
    background-color: #fff6e0;
    border: 3px solid #c99a66;
    border-radius: 10px;
    padding: 0px;
}

QPushButton[variant="card"]:hover {
    background-color: #ffeccc;
    border-color: #8a5a32;
}

QPushButton[variant="card"]:checked {
    background-color: #ffe0a3;
    border-color: #5b3a1e;
}

QPushButton[variant="card"]:pressed {
    padding: 0px;
    border-bottom-width: 3px;
}

QLabel[role="card_title"] {
    font-size: 17px;
    font-weight: bold;
}

QLabel[role="card_detail"] {
    font-size: 13px;
    color: #6b4a2a;
}

QScrollArea, QScrollArea > QWidget#qt_scrollarea_viewport, QScrollArea > QWidget > QWidget {
    background: transparent;
    border: none;
}

QScrollBar:vertical {
    background: #ead3a8;
    width: 12px;
    margin: 0px;
    border-radius: 6px;
}

QScrollBar::handle:vertical {
    background: #b07a46;
    min-height: 30px;
    border-radius: 6px;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}

QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: none;
}
)";

}

QString launcher_stylesheet() { return QString::fromUtf8(STYLESHEET); }

}
