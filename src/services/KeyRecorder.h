#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QSet>

class QKeyEvent;
namespace kcc {
class MacroService;

// Records real key presses/releases (while this app's window has focus) into a macro.
// Keys are captured by position (native key code → HID usage) so layouts and macOS's Cmd/Ctrl
// swap do not change what is recorded. While recording, key events do not reach the UI.
class KeyRecorder : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool recording READ recording NOTIFY changed)
    Q_PROPERTY(bool recordDelays READ recordDelays WRITE setRecordDelays NOTIFY changed)
    Q_PROPERTY(int recorded READ recorded NOTIFY changed)       // events added in this take
public:
    explicit KeyRecorder(MacroService* macros, QObject* parent = nullptr);
    bool recording() const { return m_macro >= 0; }
    bool recordDelays() const { return m_delays; }
    void setRecordDelays(bool on) { if (on != m_delays) { m_delays = on; emit changed(); } }
    int recorded() const { return m_count; }

    // at < 0 appends; otherwise inserts before step `at`.
    Q_INVOKABLE void start(int macroIndex, int at = -1);
    Q_INVOKABLE void stop();

    static int hidFromEvent(const QKeyEvent* e);
signals:
    void changed();
protected:
    bool eventFilter(QObject* o, QEvent* e) override;
private:
    MacroService* m_macros;
    int m_macro = -1, m_at = -1, m_count = 0;
    bool m_delays = true;
    QElapsedTimer m_clock;
    QSet<int> m_down;
};
}
