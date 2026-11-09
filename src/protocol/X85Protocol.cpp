#include "X85Protocol.h"
namespace kcc::x85 {
const ModeDef* findMode(uint8_t id) {
    for (const auto& m : kModes) if (m.id == id) return &m;
    return nullptr;
}
}
