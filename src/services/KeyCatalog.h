#pragma once
#include <QString>
#include <QVariantList>
#include <cstdint>

namespace kcc {
// Assignable targets whose slot encoding is proven by the vendor app (docs/PROTOCOL.md §6).
class KeyCatalog {
public:
    static QString hidName(uint16_t usage);
    static QString consumerName(uint16_t usage);
    static QString mouseName(uint8_t mask);
    static QVariantList keys();       // {usage, name}
    static QVariantList consumer();   // {usage, name}
    static QVariantList mouse();      // {kind:"mouse"|"wheel", value, name}
};
}
