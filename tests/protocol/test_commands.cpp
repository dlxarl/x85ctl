#include "protocol/CommandBuilder.h"
#include "protocol/X85Layout.h"
#include "services/ColorCorrection.h"
#include <QtTest>

using namespace kcc;
static QByteArray hx(const char* s) { return QByteArray::fromHex(s); }

class CommandsTest : public QObject {
    Q_OBJECT
private slots:
    void header() {
        QCOMPARE(CommandBuilder::header(0x18).left(9), hx("041800000000000000"));
        QCOMPARE(CommandBuilder::header(0x13, 1).left(9), hx("041300000000000001"));
        QCOMPARE(CommandBuilder::header(0x17, 1, 1).left(9), hx("041701000000000001"));
        QCOMPARE(CommandBuilder::header(0x02).size(), 64);
    }
    // FUN_0042b400: begin, 04 13 [8]=1, data, 04 02, 04 F0
    void lighting() {
        LightingState s; s.mode = 7; s.r = 0x11; s.g = 0x22; s.b = 0x33; s.colorful = true;
        s.brightness = 4; s.speed = 2; s.direction = x85::Direction::Up;
        const auto t = CommandBuilder::lighting(s);
        QCOMPARE(t.size(), 5);
        QCOMPARE(uint8_t(t[0].data[1]), uint8_t(0x18));
        QCOMPARE(uint8_t(t[1].data[1]), uint8_t(0x13));
        QCOMPARE(uint8_t(t[1].data[8]), uint8_t(1));
        QCOMPARE(t[2].data.left(16), hx("0711223300000000010402020000aa55"));
        QVERIFY(!t[2].readReply);
        QCOMPARE(uint8_t(t[3].data[1]), uint8_t(0x02));
        QCOMPARE(uint8_t(t[4].data[1]), uint8_t(0xF0));
        for (const auto& p : t) QCOMPARE(p.data.size(), 64);
    }
    void clampsRanges() {
        LightingState s; s.brightness = 99; s.speed = 99;
        const auto d = CommandBuilder::lighting(s)[2].data;
        QCOMPARE(int(d[9]), 5); QCOMPARE(int(d[10]), 5);
    }
    // FUN_0044c5f0: mode 0x80 transaction, then 04 23 [8]=9 with 576-byte [idx,R,G,B] table
    void perKey() {
        QMap<int, Rgb> m{{1, {0xFF, 0, 0}}, {94, {0, 0, 0xFF}}};
        const auto t = CommandBuilder::perKeyColors(m, 3);
        QCOMPARE(t.size(), 5 + 2 + 9 + 2);
        QCOMPARE(uint8_t(t[2].data[0]), uint8_t(0x80));
        QCOMPARE(int(t[2].data[9]), 3);
        QCOMPARE(uint8_t(t[6].data[1]), uint8_t(0x23));
        QCOMPARE(int(t[6].data[8]), 9);
        QByteArray table; for (int i = 7; i < 16; ++i) table += t[i].data;
        QCOMPARE(table.size(), 576);
        QCOMPARE(table.mid(4, 4), hx("01ff0000"));
        QCOMPARE(table.mid(94 * 4, 4), hx("5e0000ff"));
        QCOMPARE(table.mid(574, 2), hx("aa55"));
        QCOMPARE(uint8_t(t[17].data[1]), uint8_t(0xF0));
        QVERIFY(t[17].readReply);                                  // per-key F0 is sent with reply
        // table only (already in user mode): begin, 04 23, 9 pages, end, apply
        const auto s = CommandBuilder::perKeyColors(m, 3, false);
        QCOMPARE(s.size(), 2 + 9 + 2);
        QCOMPARE(uint8_t(s[1].data[1]), uint8_t(0x23));
        QCOMPARE(s[2].data.mid(4, 4), hx("01ff0000"));
    }
    // FUN_00418590: 04 11 (base) / 04 27 (Fn), 9 pages
    void keymap() {
        KeyAction ctrlC; ctrlC.kind = KeyAction::Kind::Key; ctrlC.modifiers = 0x01; ctrlC.usage = 0x06;
        KeyAction vol; vol.kind = KeyAction::Kind::Consumer; vol.usage = 0xE9;
        const auto t = CommandBuilder::keymap(true, {{20, ctrlC}, {21, vol}});
        QCOMPARE(uint8_t(t[1].data[1]), uint8_t(0x27));
        QByteArray table; for (int i = 2; i < 11; ++i) table += t[i].data;
        QCOMPARE(table.mid(80, 4), hx("02010600"));
        QCOMPARE(table.mid(84, 4), hx("03e90000"));
        QCOMPARE(table.mid(574, 2), hx("aa55"));
        QCOMPARE(uint8_t(CommandBuilder::keymap(false, {})[1].data[1]), uint8_t(0x11));
    }
    void keyActionEncoding() {
        KeyAction lshift; lshift.kind = KeyAction::Kind::Key; lshift.usage = 0xE1;
        QCOMPARE(lshift.encode(), hx("02020000"));                          // modifier as bit
        KeyAction m; m.kind = KeyAction::Kind::Mouse; m.button = 0x04;
        QCOMPARE(m.encode(), hx("01010400"));
        KeyAction w; w.kind = KeyAction::Kind::Wheel; w.button = 0xFF;
        QCOMPARE(w.encode(), hx("0103ff00"));
        QCOMPARE(KeyAction::decode(hx("02080700")).usage, uint16_t(0x07));   // Win+D from vendor table
        QCOMPARE(KeyAction::decode(hx("00000000")).kind, KeyAction::Kind::Default);
    }
    void colorCorrection() {
        ColorCorrection cc; cc.gamma = 2.2;                         // FastLED SMD5050 balance
        QCOMPARE(cc.apply(Rgb{255, 255, 255}), (Rgb{255, 176, 240}));
        QCOMPARE(cc.apply(Rgb{255, 0, 0}), (Rgb{255, 0, 0}));         // pure primaries keep full red
        const Rgb grey = cc.apply(Rgb{128, 128, 128});              // 255 * 0.502^2.2 = 56
        QCOMPARE(int(grey.r), 56);
        QVERIFY(grey.g < grey.r && grey.b < grey.r);
        QCOMPARE(cc.apply(Rgb{1, 0, 0}).r, uint8_t(1));            // dim but lit, never rounded to off
        for (int v : {32, 64, 128, 200, 255}) {                     // read-back inverse is close (8-bit PWM is coarse below ~32)
            const Rgb back = cc.invert(cc.apply(Rgb{uint8_t(v), uint8_t(v), uint8_t(v)}));
            QVERIFY2(qAbs(int(back.r) - v) <= 12 && qAbs(int(back.g) - v) <= 12, qPrintable(QString::number(v)));
        }
        cc.enabled = false;
        QCOMPARE(cc.apply(Rgb{128, 64, 32}), (Rgb{128, 64, 32}));
    }
    void macroUpload() {
        using T = MacroEvent::Type;
        MacroDef a; a.name = "a";
        a.events = {{T::KeyDown, 0x04}, {T::Delay, 50}, {T::KeyUp, 0x04}};
        MacroDef empty; empty.name = "empty";
        MacroDef b; b.events = {{T::MouseDown, 1}, {T::MouseUp, 1}, {T::Delay, 3}};   // delay below 10 ms is raised to 10
        QString err;
        const auto t = CommandBuilder::macros({a, empty, b}, &err);
        QVERIFY2(!t.isEmpty(), qPrintable(err));
        // len = 400 + (8 + 12) + (8 + 12) = 440 → 440/64 = 6, remainder → 8 pages (vendor adds one spare page)
        QCOMPARE(t.size(), 2 + 8 + 1);
        QCOMPARE(t[0].data.left(2), hx("0419"));                      // macro begin, not 04 18
        QCOMPARE(t[1].data.left(2), hx("0415"));
        QCOMPARE(int(uint8_t(t[1].data[8])), 8);
        QCOMPARE(t.last().data.left(2), hx("0402"));
        for (const auto& p : t) QCOMPARE(p.data.size(), 64);
        for (int i = 2; i < 10; ++i) QVERIFY(!t[i].data.isEmpty() && !t[i].readReply);
        QByteArray d; for (int i = 2; i < 10; ++i) d += t[i].data;
        QCOMPARE(d.mid(0, 4), hx("90010000"));                        // macro 0 at offset 400
        QCOMPARE(d.mid(4, 4), hx("ffffffff"));                        // macro 1 has no events
        QCOMPARE(d.mid(8, 4), hx("a4010000"));                        // macro 2 at 400 + 20 = 420
        QCOMPARE(d.mid(400, 2), hx("0300"));                          // 3 events
        QCOMPARE(d.mid(408, 12), hx("000004b0" "32000050" "00000430"));
        QCOMPARE(d.mid(420, 2), hx("0300"));
        QCOMPARE(d.mid(428, 12), hx("00000190" "00000110" "0a000050"));
        QCOMPARE(d.right(2), hx("aa55"));
        // too big: > 56 pages
        MacroDef big; for (int i = 0; i < 1000; ++i) big.events << MacroEvent{T::Delay, 10};
        QVERIFY(CommandBuilder::macros({big}, &err).isEmpty());
        QVERIFY(!err.isEmpty());
        // key slot bound to a macro: 06 index mode count
        KeyAction k; k.kind = KeyAction::Kind::Macro; k.macroIndex = 2; k.macroMode = PlayRepeat; k.macroCount = 5;
        QCOMPARE(k.encode(), hx("06020105"));
        QCOMPARE(KeyAction::decode(hx("06020105")), k);
    }
    void settings() {
        KeyboardSettings s; s.keyResponse = 3; s.sleepTime = 2;
        const auto t = CommandBuilder::settings(s);
        QCOMPARE(t.size(), 4);                       // no 04 F0 after settings
        QCOMPARE(t[2].data.left(9), hx("000100000000020003"));
        QCOMPARE(t[2].data.mid(62, 2), hx("aa55"));
    }
    void layoutIndices() {
        QCOMPARE(int(x85::layout().size()), 81);
        for (const auto& k : x85::layout()) {
            QVERIFY(k.keyIndex > 0 && k.keyIndex < x85::kSlotCount - 1);
            QCOMPARE(k.keyIndex, k.lightIndex);
        }
    }
};
QTEST_APPLESS_MAIN(CommandsTest)
#include "test_commands.moc"
