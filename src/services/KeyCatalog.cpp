#include "KeyCatalog.h"
#include <QVariantMap>

namespace kcc {
QString KeyCatalog::hidName(uint16_t u) {
    if (u >= 0x04 && u <= 0x1D) return QString(QChar('A' + (u - 0x04)));
    if (u >= 0x1E && u <= 0x26) return QString::number(u - 0x1D);
    if (u >= 0x3A && u <= 0x45) return "F" + QString::number(u - 0x39);
    if (u >= 0x68 && u <= 0x73) return "F" + QString::number(u - 0x68 + 13);
    switch (u) {
    case 0x27: return "0"; case 0x28: return "Enter"; case 0x29: return "Esc"; case 0x2A: return "Backspace";
    case 0x2B: return "Tab"; case 0x2C: return "Space"; case 0x2D: return "-"; case 0x2E: return "=";
    case 0x2F: return "["; case 0x30: return "]"; case 0x31: return "\\"; case 0x33: return ";";
    case 0x34: return "'"; case 0x35: return "`"; case 0x36: return ","; case 0x37: return ".";
    case 0x38: return "/"; case 0x39: return "Caps Lock"; case 0x46: return "Print Screen";
    case 0x47: return "Scroll Lock"; case 0x48: return "Pause"; case 0x49: return "Insert";
    case 0x4A: return "Home"; case 0x4B: return "Page Up"; case 0x4C: return "Delete"; case 0x4D: return "End";
    case 0x4E: return "Page Down"; case 0x4F: return "Right"; case 0x50: return "Left"; case 0x51: return "Down";
    case 0x52: return "Up"; case 0x65: return "Menu";
    case 0xE0: return "Left Ctrl"; case 0xE1: return "Left Shift"; case 0xE2: return "Left Alt/Option";
    case 0xE3: return "Left Win/Cmd"; case 0xE4: return "Right Ctrl"; case 0xE5: return "Right Shift";
    case 0xE6: return "Right Alt/Option"; case 0xE7: return "Right Win/Cmd";
    }
    return QString("0x%1").arg(u, 2, 16, QChar('0'));
}

QString KeyCatalog::consumerName(uint16_t u) {
    switch (u) {
    case 0xCD: return "Play / Pause"; case 0xB7: return "Stop"; case 0xB6: return "Previous track";
    case 0xB5: return "Next track"; case 0xE9: return "Volume up"; case 0xEA: return "Volume down";
    case 0xE2: return "Mute";
    }
    return QString("Consumer 0x%1").arg(u, 3, 16, QChar('0'));
}

QString KeyCatalog::mouseName(uint8_t m) {
    switch (m) { case 1: return "Left click"; case 2: return "Right click"; case 4: return "Middle click";
                 case 8: return "Back"; case 0x10: return "Forward"; }
    return QString("Mouse 0x%1").arg(m, 2, 16, QChar('0'));
}

QVariantList KeyCatalog::keys() {
    QVariantList l;
    auto add = [&](uint16_t u) { l << QVariantMap{{"usage", u}, {"name", hidName(u)}}; };
    for (uint16_t u = 0x04; u <= 0x52; ++u) if (u != 0x32) add(u);
    for (uint16_t u = 0x68; u <= 0x73; ++u) add(u);
    add(0x65);
    for (uint16_t u = 0xE0; u <= 0xE7; ++u) add(u);
    return l;
}
QVariantList KeyCatalog::consumer() {
    QVariantList l;
    for (uint16_t u : {0xCD, 0xB5, 0xB6, 0xB7, 0xE9, 0xEA, 0xE2})
        l << QVariantMap{{"usage", u}, {"name", consumerName(u)}};
    return l;
}
QVariantList KeyCatalog::mouse() {
    QVariantList l;
    for (uint8_t m : {1, 2, 4, 8, 0x10}) l << QVariantMap{{"kind", "mouse"}, {"value", m}, {"name", mouseName(m)}};
    l << QVariantMap{{"kind", "wheel"}, {"value", 0x01}, {"name", "Wheel up"}}
      << QVariantMap{{"kind", "wheel"}, {"value", 0xFF}, {"name", "Wheel down"}};
    return l;
}
}
