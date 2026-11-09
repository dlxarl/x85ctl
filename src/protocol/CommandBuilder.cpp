#include "CommandBuilder.h"

namespace kcc {
using namespace x85;

static QByteArray zero(int n = kPacketSize) { return QByteArray(n, '\0'); }

QByteArray CommandBuilder::header(uint8_t cmd, uint8_t pages, uint8_t b2) {
    QByteArray p = zero();
    p[0] = char(kPrefix); p[1] = char(cmd); p[2] = char(b2); p[8] = char(pages);
    return p;
}

QVector<QByteArray> CommandBuilder::pages(QByteArray payload, int n) {
    payload.resize(n * kPacketSize, '\0');
    payload[n * kPacketSize - 2] = char(0xAA);
    payload[n * kPacketSize - 1] = char(0x55);
    QVector<QByteArray> out;
    for (int i = 0; i < n; ++i) out << payload.mid(i * kPacketSize, kPacketSize);
    return out;
}

static void frame(Transaction& t, const QByteArray& hdr, const QVector<QByteArray>& data, bool apply,
                  bool applyReadsReply = false) {
    t << Packet{CommandBuilder::header(Begin)} << Packet{hdr};
    for (const auto& d : data) t << Packet{d, false};   // FUN_0044faa0: data pages are not read back
    t << Packet{CommandBuilder::header(End)};
    if (apply) t << Packet{CommandBuilder::header(Apply), applyReadsReply};
}

Transaction CommandBuilder::lighting(const LightingState& s) {
    QByteArray d = zero(16);
    d[0] = char(s.mode); d[1] = char(s.r); d[2] = char(s.g); d[3] = char(s.b);
    d[8] = char(s.colorful ? 1 : 0);
    d[9] = char(qMin<int>(s.brightness, kBrightnessMax));
    d[10] = char(qMin<int>(s.speed, kSpeedMax));
    d[11] = char(s.direction);
    d[14] = char(0xAA); d[15] = char(0x55);   // single page: trailer at 14..15, not 62..63
    d.resize(kPacketSize, '\0');
    Transaction t;
    frame(t, header(LightMode, 1), {d}, true);
    return t;
}

Transaction CommandBuilder::perKeyColors(const QMap<int, Rgb>& c, uint8_t brightness, bool switchMode) {
    // FUN_0044c5f0: first switch to user mode 0x80 (FUN_0044c4e0), then upload the table.
    Transaction t;
    if (switchMode) {
        QByteArray m = zero(16);
        m[0] = char(kModeUser); m[9] = char(qMin<int>(brightness, kBrightnessMax));
        m[14] = char(0xAA); m[15] = char(0x55);
        m.resize(kPacketSize, '\0');
        frame(t, header(LightMode, 1), {m}, true);
    }

    QByteArray table = zero(kTableBytes);
    for (auto it = c.begin(); it != c.end(); ++it) {
        const int i = it.key();
        if (i <= 0 || i >= kSlotCount - 1) continue;   // slot 143 overlaps the AA55 trailer
        table[i * 4] = char(i);
        table[i * 4 + 1] = char(it->r); table[i * 4 + 2] = char(it->g); table[i * 4 + 3] = char(it->b);
    }
    frame(t, header(PerKeyColor, 9), pages(table, 9), true, true);   // FUN_0044c5f0 reads the F0 reply
    return t;
}

Transaction CommandBuilder::keymap(bool fn, const QMap<int, KeyAction>& k) {
    QByteArray table = zero(kTableBytes);
    for (auto it = k.begin(); it != k.end(); ++it) {
        const int i = it.key();
        if (i < 0 || i >= kSlotCount - 1) continue;
        table.replace(i * 4, 4, it->encode());
    }
    Transaction t;
    frame(t, header(fn ? KeymapFn : KeymapBase, 9), pages(table, 9), true);
    return t;
}

Transaction CommandBuilder::settings(const KeyboardSettings& s) {
    QByteArray d = zero();
    d[1] = 1;
    d[2] = char(s.option1); d[3] = char(s.option2); d[4] = char(s.option3);
    d[5] = char(s.fnSwitch); d[6] = char(s.sleepTime); d[8] = char(s.keyResponse);
    d[62] = char(0xAA); d[63] = char(0x55);
    Transaction t;
    frame(t, header(Settings, 1, 1), {d}, false);   // FUN_00413ff0 sends no 04 F0
    return t;
}

// Layout of the 0x15 block (FUN_0042db70):
//   [0..399]  100 index entries: u16 offset LE + 00 00 (FF FF FF FF = macro without events)
//   at offset: u16 event count, 6 zero bytes, then count × 4-byte events
//   pages = len/64 + (len % 64 ? 2 : 1), AA 55 in the last two bytes
static int eventBytes(const MacroDef& m) { return m.events.isEmpty() ? 0 : 8 + 4 * int(m.events.size()); }
int CommandBuilder::macroBytes(const QVector<MacroDef>& list) {
    int n = kMacroMaxCount * 4;
    for (const auto& m : list) n += eventBytes(m);
    return n;
}

Transaction CommandBuilder::macros(const QVector<MacroDef>& list, QString* error) {
    auto fail = [&](const QString& e) { if (error) *error = e; return Transaction{}; };
    if (list.size() > kMacroMaxCount) return fail("Too many macros (max 100)");
    const int len = macroBytes(list);
    const int pageCount = len / kPacketSize + (len % kPacketSize ? 2 : 1);
    if (pageCount * kPacketSize > kMacroBufferBytes || pageCount > 255) return fail("The macros do not fit in the keyboard's macro memory");
    QByteArray d(pageCount * kPacketSize, '\0');
    int off = kMacroMaxCount * 4;
    for (int i = 0; i < list.size(); ++i) {
        const MacroDef& m = list[i];
        if (m.events.isEmpty()) { for (int b = 0; b < 4; ++b) d[i * 4 + b] = char(0xFF); continue; }
        d[i * 4] = char(off & 0xFF); d[i * 4 + 1] = char(off >> 8);
        d[off] = char(m.events.size() & 0xFF); d[off + 1] = char((m.events.size() >> 8) & 0xFF);
        int e = off + 8;
        for (const auto& ev : m.events) {
            switch (ev.type) {
            case MacroEvent::Type::KeyDown:   d[e + 2] = char(ev.value); d[e + 3] = char(0xB0); break;
            case MacroEvent::Type::KeyUp:     d[e + 2] = char(ev.value); d[e + 3] = char(0x30); break;
            case MacroEvent::Type::MouseDown: d[e + 2] = char(ev.value); d[e + 3] = char(0x90); break;
            case MacroEvent::Type::MouseUp:   d[e + 2] = char(ev.value); d[e + 3] = char(0x10); break;
            case MacroEvent::Type::Delay: {
                const int ms = qBound(10, int(ev.value), 0xFFFF);
                d[e] = char(ms & 0xFF); d[e + 1] = char(ms >> 8); d[e + 3] = char(0x50); break; }
            }
            e += 4;
        }
        off = e;
    }
    Transaction t;
    t << Packet{header(BeginMacro)} << Packet{header(Macro, uint8_t(pageCount))};
    for (const auto& pg : pages(d, pageCount)) t << Packet{pg, false};
    t << Packet{header(End)};
    return t;
}

Transaction CommandBuilder::readPerKeyColorsRequest() {
    return {Packet{header(ReadPerKeyColor, 9)}};
}

QByteArray KeyAction::encode() const {
    QByteArray s(4, '\0');
    switch (kind) {
    case Kind::Default: break;
    case Kind::Key:
        s[0] = 0x02;
        if (usage >= 0xE0 && usage <= 0xE7 && modifiers == 0) s[1] = char(1u << (usage - 0xE0));
        else { s[1] = char(modifiers); s[2] = char(usage); }
        break;
    case Kind::Consumer: s[0] = 0x03; s[1] = char(usage & 0xFF); s[2] = char(usage >> 8); break;
    case Kind::Mouse:    s[0] = 0x01; s[1] = 0x01; s[2] = char(button); break;
    case Kind::Wheel:    s[0] = 0x01; s[1] = 0x03; s[2] = char(button); break;
    case Kind::Macro:    s[0] = 0x06; s[1] = char(macroIndex); s[2] = char(macroMode); s[3] = char(macroCount); break;
    }
    return s;
}

KeyAction KeyAction::decode(const QByteArray& s) {
    KeyAction a;
    if (s.size() < 4) return a;
    const uint8_t t = uint8_t(s[0]), b1 = uint8_t(s[1]), b2 = uint8_t(s[2]);
    if (t == 0x02) { a.kind = Kind::Key; a.modifiers = b1; a.usage = b2; }
    else if (t == 0x03) { a.kind = Kind::Consumer; a.usage = uint16_t(b1 | (b2 << 8)); }
    else if (t == 0x01 && b1 == 0x01) { a.kind = Kind::Mouse; a.button = b2; }
    else if (t == 0x01 && b1 == 0x03) { a.kind = Kind::Wheel; a.button = b2; }
    else if (t == 0x06) { a.kind = Kind::Macro; a.macroIndex = b1; a.macroMode = b2; a.macroCount = uint8_t(s[3]); }
    return a;
}
}
