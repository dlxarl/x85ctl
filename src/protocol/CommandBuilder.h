#pragma once
#include "X85Protocol.h"
#include <QByteArray>
#include <QMap>
#include <QString>
#include <QVector>

namespace kcc {

// One 64-byte feature report and whether the vendor app reads the reply after it.
struct Packet {
    QByteArray data;       // exactly 64 bytes, no report ID
    bool readReply = true;
};
using Transaction = QVector<Packet>;

struct LightingState {
    uint8_t mode = x85::kDefaultMode;
    uint8_t brightness = x85::kDefaultBrightness;
    uint8_t speed = x85::kDefaultSpeed;
    x85::Direction direction = x85::Direction::Left;
    bool colorful = false;
    uint8_t r = 0xFF, g = 0xFF, b = 0xFF;
    bool operator==(const LightingState&) const = default;
};

struct Rgb { uint8_t r = 0, g = 0, b = 0; bool operator==(const Rgb&) const = default; };

// Keymap slot (4 bytes). Only the kinds whose encoding is proven are exposed.
struct KeyAction {
    enum class Kind : uint8_t { Default, Key, Consumer, Mouse, Wheel, Macro };
    Kind kind = Kind::Default;
    uint8_t modifiers = 0;  // HID modifier bitmask (Key)
    uint16_t usage = 0;     // HID usage (Key) / consumer usage (Consumer)
    uint8_t button = 0;     // mouse button mask (Mouse) / wheel delta 0x01 or 0xFF (Wheel)
    uint8_t macroIndex = 0; // Macro: position in the uploaded macro table (0x15)
    uint8_t macroMode = 0;  // Macro: see MacroPlay; only "repeat" (1) is proven by the vendor code
    uint8_t macroCount = 0; // Macro: repeat count 1..255 when macroMode == 1
    QByteArray encode() const;
    static KeyAction decode(const QByteArray& slot4);
    bool operator==(const KeyAction&) const = default;
};

// How a key bound to a macro plays it (slot byte 2). Values come from the vendor dialog's combo box
// (Play once / Play multiple times / Stop when pressed again / Release to stop); only 1 is proven.
enum MacroPlay : uint8_t { PlayOnce = 0, PlayRepeat = 1, PlayToggle = 2, PlayWhileHeld = 3 };

// One step of a macro as stored on the keyboard (4 bytes: [delay lo, delay hi, usage, type], FUN_0042db70).
struct MacroEvent {
    enum class Type : uint8_t { Delay, KeyDown, KeyUp, MouseDown, MouseUp };
    Type type = Type::Delay;
    uint16_t value = 0;   // Delay: ms (≥10) · Key*: HID usage (1 byte) · Mouse*: 01 left, 02 right, 04 middle
    bool operator==(const MacroEvent&) const = default;
};
struct MacroDef {
    QString name;
    QVector<MacroEvent> events;
    bool operator==(const MacroDef&) const = default;
};

struct KeyboardSettings {
    // Byte meanings for 2..4 are inferred (see docs/PROTOCOL.md §7).
    bool option1 = false, option2 = false, option3 = false;
    uint8_t fnSwitch = 0, sleepTime = 0, keyResponse = 1;
};

// Builds byte-exact transactions matching the vendor app's sequences.
class CommandBuilder {
public:
    static QByteArray header(uint8_t cmd, uint8_t pages = 0, uint8_t b2 = 0);
    static Transaction lighting(const LightingState& s);
    // Full upload as the vendor app does it: switch to user mode 0x80, then the 04 23 colour table.
    // switchMode=false sends only the table (board already in user mode) and avoids a visible blink.
    static Transaction perKeyColors(const QMap<int, Rgb>& byLightIndex, uint8_t brightness, bool switchMode = true);
    static Transaction keymap(bool fnLayer, const QMap<int, KeyAction>& byKeyIndex);
    static Transaction settings(const KeyboardSettings& s);
    // Macro table upload (04 19 begin, 04 15, data, 04 02; no 04 F0). Empty result + *error when it does not fit.
    static Transaction macros(const QVector<MacroDef>& list, QString* error = nullptr);
    static constexpr int kMacroMaxCount = 100;      // index table: 400 bytes
    static constexpr int kMacroBufferBytes = 3584;  // vendor buffer (0xE00)
    static int macroBytes(const QVector<MacroDef>& list);   // bytes used incl. the 400-byte index table
    static Transaction readPerKeyColorsRequest();   // header only; caller reads pages
    // Splits a payload into 64-byte pages and stamps AA 55 at the end.
    static QVector<QByteArray> pages(QByteArray payload, int pageCount);
};
}
