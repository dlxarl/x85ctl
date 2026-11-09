#include "core/DeviceManager.h"
#include "services/LightingService.h"
#include "platform/GlobalHotkeys.h"
#include "protocol/X85Layout.h"
#include "services/SnakeGame.h"
#include <QtTest>

using namespace kcc;

class TestSnake : public QObject {
    Q_OBJECT
private slots:
    void gridWrapsAndIsConsistent() {
        DeviceManager dev; LightingService light(&dev); SnakeGame g(&dev, &light);
        QCOMPARE(g.cellCount(), 81);
        for (int i = 0; i < g.cellCount(); ++i) {
            for (int d = 0; d < 4; ++d) { const int n = g.neighbour(i, d); QVERIFY(n >= 0 && n < g.cellCount()); }
            QCOMPARE(g.neighbour(g.neighbour(i, SnakeGame::Right), SnakeGame::Left), i);   // left/right are exact inverses
        }
        // walking right through a whole row comes back to the start
        int c = 0, steps = 0; do { c = g.neighbour(c, SnakeGame::Right); ++steps; } while (c != 0 && steps < 100);
        QVERIFY(steps > 10 && steps < 20);
    }
    void startsSmallGrowsByOneAndWrapsThroughTheEdge() {
        DeviceManager dev; LightingService light(&dev); SnakeGame g(&dev, &light);
        g.seed(1); g.beginRun();
        QCOMPARE(g.length(), 1);
        const int head0 = g.body().first();
        g.placeFood(g.neighbour(head0, SnakeGame::Right));
        QVERIFY(g.step());
        QCOMPARE(g.length(), 2); QCOMPARE(g.score(), 1);
        g.placeFood(-1);
        // keep going right: it must never end by hitting an edge, it wraps
        for (int i = 0; i < 60; ++i) QVERIFY(g.step());
        QCOMPARE(g.length(), 2);
        QVERIFY(g.running());
        g.turn(SnakeGame::Left);                                         // reversal: ignored
        QVERIFY(g.step());
        QCOMPARE(g.headDir(), int(SnakeGame::Right));
    }
    void refusesWithoutKeyboard() {
        DeviceManager dev; LightingService light(&dev); SnakeGame g(&dev, &light);
        QVERIFY(!g.canPlay());
        QVERIFY(!g.start());
        QVERIFY(!g.running());
        QVERIFY(!g.message().isEmpty());
    }
    void arrowsAreLit() {
        DeviceManager dev; LightingService light(&dev); SnakeGame g(&dev, &light);
        const auto c = g.colors();
        int lit = 0; for (const auto& k : x85::layout()) if (k.hidUsage >= 0x4F && k.hidUsage <= 0x52 && c.contains(QString::number(k.lightIndex))) ++lit;
        QCOMPARE(lit, 4);
    }
    void hotkeyRegistersAndUnregisters() {
        auto& h = GlobalHotkeys::instance();
        if (!GlobalHotkeys::supported()) QSKIP("system-wide shortcuts are macOS-only");
        const quint32 mods = GlobalHotkeys::modsFromQt(Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier);
        QVERIFY(h.add(77, 0x6F, mods));            // ⌃⌥⌘F12
        QVERIFY(h.has(77));
        h.remove(77);
        QVERIFY(!h.has(77));
    }
    void biteOwnTailEnds() {
        DeviceManager dev; LightingService light(&dev); SnakeGame g(&dev, &light);
        g.seed(7); g.beginRun();
        for (int k = 0; k < 4; ++k) { g.placeFood(g.neighbour(g.body().first(), SnakeGame::Right)); QVERIFY(g.step()); }
        QCOMPARE(g.length(), 5);
        g.placeFood(-1);
        g.turn(SnakeGame::Up); QVERIFY(g.step());
        g.turn(SnakeGame::Left); QVERIFY(g.step());
        g.turn(SnakeGame::Down); QVERIFY(!g.step());                      // lands on the body
        QVERIFY(g.over());
        QVERIFY(g.best() >= 4);
    }
};
QTEST_MAIN(TestSnake)
#include "test_snake.moc"
