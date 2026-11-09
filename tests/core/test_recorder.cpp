#include "services/KeyRecorder.h"
#include "services/MacroService.h"
#include "core/DeviceManager.h"
#include <QKeyEvent>
#include <QWindow>
#include <QtTest>

using namespace kcc;

class TestRecorder : public QObject {
    Q_OBJECT
    static void key(QWindow& w, QEvent::Type t, Qt::Key k, quint32 nativeVk, bool repeat = false) {
        QKeyEvent e(t, k, Qt::NoModifier, 0, nativeVk, 0, QString(), repeat);
        QCoreApplication::sendEvent(&w, &e);
    }
private slots:
    void recordsKeysAndReleasesHeldOnes() {
        DeviceManager dev;
        MacroService m(&dev);
        KeyRecorder r(&m);
        r.setRecordDelays(false);
        const int i = m.add("t");
        QWindow w;
        r.start(i);
        QVERIFY(r.recording());
#ifdef Q_OS_MACOS
        const quint32 vkShift = 0x38, vkA = 0x00, vkCmd = 0x37;
#else
        const quint32 vkShift = 0, vkA = 0, vkCmd = 0;
#endif
        key(w, QEvent::KeyPress, Qt::Key_Shift, vkShift);
        key(w, QEvent::KeyPress, Qt::Key_A, vkA);
        key(w, QEvent::KeyPress, Qt::Key_A, vkA, true);       // auto-repeat: ignored
        key(w, QEvent::KeyRelease, Qt::Key_A, vkA);
        key(w, QEvent::KeyPress, Qt::Key_Control, vkCmd);       // still held when recording stops
        r.stop();
        QVERIFY(!r.recording());
        using T = MacroEvent::Type;
        const auto& ev = m.list()[i].events;
        const int gui = 0xE3;   // macOS: ⌘ = Key_Control; elsewhere Key_Control = Ctrl (0xE0)
#ifdef Q_OS_MACOS
        const int cmd = gui;
#else
        const int cmd = 0xE0;
#endif
        const QVector<MacroEvent> want = {{T::KeyDown, 0xE1}, {T::KeyDown, 0x04}, {T::KeyUp, 0x04}, {T::KeyDown, uint16_t(cmd)},
                                          {T::KeyUp, 0xE1}, {T::KeyUp, uint16_t(cmd)}};
        QCOMPARE(ev.size(), want.size());
        // the two releases added on stop come from a set: compare them order-independently
        for (int k = 0; k < 4; ++k) QCOMPARE(ev[k], want[k]);
        QVERIFY((ev[4] == want[4] && ev[5] == want[5]) || (ev[4] == want[5] && ev[5] == want[4]));
        Q_UNUSED(gui);
    }
    void recordsDelaysAndInserts() {
        DeviceManager dev;
        MacroService m(&dev);
        const int i = m.add("t");
        m.addStep(i, "key", 0x05);                               // B down, B up
        KeyRecorder r(&m);
        QWindow w;
        r.start(i, 1);                                           // between B down and B up
        key(w, QEvent::KeyPress, Qt::Key_C, 0x08);
        QTest::qWait(40);
        key(w, QEvent::KeyRelease, Qt::Key_C, 0x08);
        r.stop();
        const auto& ev = m.list()[i].events;
        QCOMPARE(ev.size(), 5);
        QCOMPARE(int(ev[1].value), 0x06);
        QCOMPARE(ev[2].type, MacroEvent::Type::Delay);
        QVERIFY(ev[2].value >= 30);
        QCOMPARE(int(ev[4].value), 0x05);                        // original B up moved to the end
    }
};
QTEST_MAIN(TestRecorder)
#include "test_recorder.moc"
