#pragma once
#include <cstdint>
#include <vector>

namespace kcc::x85 {
// One physical key. Geometry is in the pixel space of ui/assets/x85.png.
struct KeyDef {
    const char* label;
    uint8_t hidUsage;   // default HID usage (0xAF = Fn, not a real usage)
    int keyIndex;       // slot in keymap tables (cmd 0x11/0x27)
    int lightIndex;     // slot in per-key colour table (cmd 0x23)
    int row, col;
    int x, y, w, h;
};
const std::vector<KeyDef>& layout();
inline constexpr int kCanvasWidth = 1140;
inline constexpr int kCanvasHeight = 520;
inline constexpr int kKnobX = 1080, kKnobY = 87, kKnobR = 31;   // rotary knob (not a key)
}
