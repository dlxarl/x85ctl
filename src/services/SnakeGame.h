#pragma once
#include "protocol/CommandBuilder.h"
#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <QVector>
#include <random>

namespace kcc {
class DeviceManager;
class LightingService;

// Snake played on the keys themselves: every key is a cell, neighbours come from the physical layout.
// It is played on the real LEDs (per-key table, same 04 23 transaction as the editor; the arrow keys are lit
// as a hint) and mirrored on the keyboard photo. When the game ends the previous lighting is re-applied.
// A user-chosen shortcut starts/stops it from anywhere; while it runs the arrow keys steer it system-wide.
class SnakeGame : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(bool over READ over NOTIFY changed)
    Q_PROPERTY(int score READ score NOTIFY changed)
    Q_PROPERTY(int best READ best NOTIFY changed)
    Q_PROPERTY(int length READ length NOTIFY changed)
    Q_PROPERTY(int intervalMs READ intervalMs WRITE setIntervalMs NOTIFY changed)
    Q_PROPERTY(bool canPlay READ canPlay NOTIFY changed)               // keyboard connected and writes enabled
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString hotkeyLabel READ hotkeyLabel NOTIFY changed)    // "" = not set
    Q_PROPERTY(bool capturing READ capturing NOTIFY changed)
    Q_PROPERTY(bool hotkeySupported READ hotkeySupported CONSTANT)
    Q_PROPERTY(QVariantMap colors READ colors NOTIFY frame)          // lightIndex -> colour, for the on-screen keyboard
public:
    enum Dir { Up = 0, Right = 1, Down = 2, Left = 3 };
    SnakeGame(DeviceManager* dev, LightingService* light, QObject* parent = nullptr);

    bool running() const { return m_running; }
    bool over() const { return m_over; }
    int score() const { return m_score; }
    int best() const { return m_best; }
    int length() const { return int(m_body.size()); }
    int intervalMs() const { return m_interval; }
    void setIntervalMs(int ms);
    bool canPlay() const;
    QString message() const { return m_message; }
    QString hotkeyLabel() const { return m_hotkeyLabel; }
    bool capturing() const { return m_capturing; }
    static bool hotkeySupported();
    QVariantMap colors() const;

    Q_INVOKABLE bool start();              // false when the keyboard is not connected / writes are off
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void captureHotkey();      // the next key press with a modifier becomes the start/stop shortcut
    Q_INVOKABLE void cancelCapture();
    Q_INVOKABLE void clearHotkey();
    Q_INVOKABLE void stop();               // also restores the keyboard lighting when it was mirrored
    Q_INVOKABLE void turn(int dir);        // Up/Right/Down/Left; a 180° turn is ignored

    // --- engine, exposed for tests ---
    void setHotkey(int qtKey, int qtModifiers, quint32 nativeVk);   // also persists and registers it
    void beginRun();                       // start without the keyboard checks (tests)
    void seed(unsigned s) { m_rng.seed(s); }
    bool step();                           // false when the game ended on this step
    int cellCount() const { return int(m_cells.size()); }
    int neighbour(int cell, int dir) const { return m_cells[cell].nb[dir]; }
    int cellLight(int cell) const { return m_cells[cell].light; }
    const QVector<int>& body() const { return m_body; }   // head first
    int food() const { return m_food; }
    void placeFood(int cell) { m_food = cell; }
    int headDir() const { return m_dir; }
signals:
    void changed();
    void frame();
    void gameOver(int score);
private:
    struct Cell { int light = 0; int row = 0; double cx = 0; int nb[4] = {-1, -1, -1, -1}; };
    void buildGrid();
    void reset();
    void pickFood();
    void finish();
    void setMessage(const QString& m) { if (m != m_message) { m_message = m; emit changed(); } }
    void registerHotkey();
    void registerSteering(bool on);
    void render();
    QMap<int, Rgb> frameColors() const;

    DeviceManager* m_dev;
    LightingService* m_light;
    QVector<Cell> m_cells;
    QVector<int> m_body;
    QVector<int> m_queue;               // pending turns (max 2)
    int m_dir = Right, m_food = -1, m_score = 0, m_best = 0, m_interval = 600;
    bool m_running = false, m_over = false, m_boardInGame = false, m_capturing = false;
    QString m_message, m_hotkeyLabel;
    quint32 m_hotkeyVk = 0; int m_hotkeyMods = 0;
    QVector<int> m_arrows;               // light indices of the four arrow keys
    QTimer m_timer;
protected:
    bool eventFilter(QObject* o, QEvent* e) override;
private:
    std::mt19937 m_rng{std::random_device{}()};
};
}
