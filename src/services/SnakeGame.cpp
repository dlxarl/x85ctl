#include "SnakeGame.h"
#include "LightingService.h"
#include "core/DeviceManager.h"
#include "platform/GlobalHotkeys.h"
#include "protocol/X85Layout.h"
#include "storage/SettingsRepository.h"
#include <QColor>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <algorithm>
#include <cmath>

namespace kcc {
static constexpr const char* kBestKey = "snake/best";
enum HotkeyId { HkToggle = 1, HkUp = 10, HkRight, HkDown, HkLeft, HkEsc };
// macOS virtual key codes
static constexpr quint32 kVkUp = 0x7E, kVkDown = 0x7D, kVkLeft = 0x7B, kVkRight = 0x7C, kVkEsc = 0x35;
static const Rgb kArrowColour{150, 0, 255};   // arrow keys are lit violet as the controls hint

SnakeGame::SnakeGame(DeviceManager* d, LightingService* l, QObject* p) : QObject(p), m_dev(d), m_light(l) {
    buildGrid();
    SettingsRepository st;
    m_best = st.value(kBestKey, 0).toInt();
    m_timer.setSingleShot(false);
    connect(&m_timer, &QTimer::timeout, this, [this] { step(); });
    connect(d, &DeviceManager::changed, this, [this] {
        if (m_running && !canPlay()) { m_boardInGame = false; setMessage("Keyboard disconnected — game stopped"); stop(); }
        else emit changed();
    });
    connect(&GlobalHotkeys::instance(), &GlobalHotkeys::activated, this, [this](int id) {
        switch (id) {
        case HkToggle: toggle(); break;
        case HkUp: turn(Up); break;
        case HkRight: turn(Right); break;
        case HkDown: turn(Down); break;
        case HkLeft: turn(Left); break;
        case HkEsc: stop(); break;
        }
    });
    // saved shortcut
    m_hotkeyVk = st.value("snake/hotkey/vk", 0).toUInt();
    m_hotkeyMods = st.value("snake/hotkey/mods", 0).toInt();
    m_hotkeyLabel = st.value("snake/hotkey/label").toString();
    if (!qEnvironmentVariableIsSet("KCC_SNAPSHOT_DIR")) registerHotkey();   // snapshot runs never grab system hotkeys
}

bool SnakeGame::hotkeySupported() { return GlobalHotkeys::supported(); }

void SnakeGame::buildGrid() {
    const auto& lay = x85::layout();
    m_cells.resize(int(lay.size()));
    for (int i = 0; i < m_cells.size(); ++i) {
        m_cells[i].light = lay[i].lightIndex; m_cells[i].row = lay[i].row; m_cells[i].cx = lay[i].x + lay[i].w / 2.0;
        switch (lay[i].hidUsage) {   // HID 0x52 up, 0x51 down, 0x50 left, 0x4F right
        case 0x52: case 0x51: case 0x50: case 0x4F: m_arrows << lay[i].lightIndex; break;
        }
    }
    int maxRow = 0; for (const auto& c : m_cells) maxRow = std::max(maxRow, c.row);
    QVector<QVector<int>> rows(maxRow + 1);
    for (int i = 0; i < m_cells.size(); ++i) rows[m_cells[i].row] << i;
    for (auto& r : rows) std::sort(r.begin(), r.end(), [&](int a, int b) { return m_cells[a].cx < m_cells[b].cx; });
    for (int r = 0; r < rows.size(); ++r) {
        for (int k = 0; k < rows[r].size(); ++k) {
            Cell& c = m_cells[rows[r][k]];
            // the edges wrap: the last key of a row is next to the first one
            c.nb[Left] = rows[r][(k + rows[r].size() - 1) % rows[r].size()];
            c.nb[Right] = rows[r][(k + 1) % rows[r].size()];
            auto nearest = [&](int rr) {          // closest key by centre in a neighbouring row
                int best = -1; double bd = 1e9;
                rr = (rr + int(rows.size())) % int(rows.size());       // top and bottom wrap too
                for (int j : rows[rr]) { const double d = std::abs(m_cells[j].cx - c.cx); if (d < bd) { bd = d; best = j; } }
                return best;
            };
            c.nb[Up] = nearest(r - 1);
            c.nb[Down] = nearest(r + 1);
        }
    }
}

bool SnakeGame::canPlay() const { return m_dev->ready() && m_dev->writesEnabled(); }

void SnakeGame::setIntervalMs(int ms) {
    ms = qBound(120, ms, 1500);
    if (ms == m_interval) return;
    m_interval = ms;
    if (m_running) m_timer.setInterval(m_interval);
    emit changed();
}

void SnakeGame::reset() {
    m_body.clear(); m_queue.clear(); m_score = 0; m_over = false; m_dir = Right;
    // start on the middle of the home row (row 3 is the A-row), heading right
    int head = -1; double bd = 1e9;
    for (int i = 0; i < m_cells.size(); ++i) if (m_cells[i].row == 3) {
        const double d = std::abs(m_cells[i].cx - x85::kCanvasWidth / 2.0);
        if (d < bd) { bd = d; head = i; }
    }
    if (head < 0) head = 0;
    m_body << head;   // starts as a single key; every apple adds one
    pickFood();
}

void SnakeGame::pickFood() {
    QVector<int> free;
    for (int i = 0; i < m_cells.size(); ++i) if (!m_body.contains(i)) free << i;
    if (free.isEmpty()) { m_food = -1; return; }
    m_food = free[int(m_rng() % unsigned(free.size()))];
}

bool SnakeGame::start() {
    if (m_running) return true;
    if (!canPlay()) { setMessage(m_dev->ready() ? "Switch on “Writing to keyboard” (top right) to play" : "Connect the keyboard to play"); return false; }
    beginRun();
    return true;
}

void SnakeGame::beginRun() {
    reset();
    m_message.clear();
    m_running = true; m_boardInGame = false;
    m_timer.setInterval(m_interval);
    m_timer.start();
    registerSteering(true);
    emit changed(); render();
}

void SnakeGame::toggle() { if (m_running) stop(); else start(); }

void SnakeGame::stop() {
    const bool was = m_running;
    m_timer.stop(); m_running = false;
    registerSteering(false);
    if (m_boardInGame) { m_boardInGame = false; m_light->applyCurrent(); }   // back to what the editor/profile has
    if (was) { emit changed(); emit frame(); }
}

void SnakeGame::finish() {
    m_over = true;
    if (m_score > m_best) { m_best = m_score; SettingsRepository().set(kBestKey, m_best); }
    const int s = m_score;
    stop();
    emit gameOver(s);
}

void SnakeGame::turn(int dir) {
    if (!m_running || dir < 0 || dir > 3) return;
    const int last = m_queue.isEmpty() ? m_dir : m_queue.last();
    if (dir == last || dir == (last + 2) % 4) return;   // same direction or a reversal
    if (m_queue.size() < 2) m_queue << dir;
}

bool SnakeGame::step() {
    if (!m_running) return false;
    if (!m_queue.isEmpty()) m_dir = m_queue.takeFirst();
    const int next = m_cells[m_body.first()].nb[m_dir];
    const bool eats = next >= 0 && next == m_food;
    // the tail cell is free this step unless the snake grows
    const bool hitsBody = next >= 0 && m_body.contains(next) && !(next == m_body.last() && !eats);
    if (next < 0 || hitsBody) { finish(); return false; }
    m_body.prepend(next);
    if (eats) { ++m_score; pickFood(); if (m_food < 0) { finish(); return false; } }
    else m_body.removeLast();
    emit changed(); render();
    return true;
}

QMap<int, Rgb> SnakeGame::frameColors() const {
    QMap<int, Rgb> m;
    for (int a : m_arrows) m[a] = kArrowColour;                     // controls hint, under the snake and the apple
    const int n = int(m_body.size());
    for (int k = 0; k < n; ++k) {
        const double t = n > 1 ? double(k) / (n - 1) : 0;          // 0 head … 1 tail
        const QColor c = k == 0 ? QColor(255, 255, 255) : QColor::fromHsvF(0.34, 1.0, 1.0 - 0.3 * t);
        m[m_cells[m_body[k]].light] = Rgb{uint8_t(c.red()), uint8_t(c.green()), uint8_t(c.blue())};
    }
    if (m_food >= 0) m[m_cells[m_food].light] = Rgb{255, 0, 0};
    return m;
}

QVariantMap SnakeGame::colors() const {
    QVariantMap v;
    const auto m = frameColors();
    for (auto it = m.begin(); it != m.end(); ++it) v[QString::number(it.key())] = QColor(it->r, it->g, it->b);
    return v;
}

void SnakeGame::render() {
    emit frame();
    if (!m_running || !canPlay()) return;
    // First frame: the vendor sequence (switch to user mode 0x80); the rest: table only.
    const auto tx = CommandBuilder::perKeyColors(m_light->correction().apply(frameColors()), uint8_t(x85::kBrightnessMax), !m_boardInGame);
    if (m_dev->submit(tx, "Snake", "snake")) m_boardInGame = true;
}

// ---------------- shortcut ----------------

void SnakeGame::registerHotkey() {
    GlobalHotkeys::instance().remove(HkToggle);
    if (m_hotkeyVk == 0 && m_hotkeyLabel.isEmpty()) return;
    if (!GlobalHotkeys::instance().add(HkToggle, m_hotkeyVk, GlobalHotkeys::modsFromQt(m_hotkeyMods)))
        setMessage("Could not register the shortcut " + m_hotkeyLabel + " — it may be taken by another app");
}

// While a game runs the arrow keys and Esc steer/stop it even when another app is in front.
void SnakeGame::registerSteering(bool on) {
    auto& g = GlobalHotkeys::instance();
    if (qEnvironmentVariableIsSet("KCC_SNAPSHOT_DIR")) return;
    const struct { int id; quint32 vk; } keys[] = {{HkUp, kVkUp}, {HkRight, kVkRight}, {HkDown, kVkDown}, {HkLeft, kVkLeft}, {HkEsc, kVkEsc}};
    for (const auto& k : keys) { if (on) g.add(k.id, k.vk, 0); else g.remove(k.id); }
}

void SnakeGame::setHotkey(int qtKey, int mods, quint32 vk) {
    m_hotkeyVk = vk; m_hotkeyMods = mods;
    m_hotkeyLabel = QKeySequence(mods | qtKey).toString(QKeySequence::NativeText);
    SettingsRepository st;
    st.set("snake/hotkey/vk", m_hotkeyVk); st.set("snake/hotkey/mods", m_hotkeyMods); st.set("snake/hotkey/label", m_hotkeyLabel);
    m_message.clear();
    registerHotkey();
    emit changed();
}

void SnakeGame::clearHotkey() {
    m_hotkeyVk = 0; m_hotkeyMods = 0; m_hotkeyLabel.clear();
    SettingsRepository st;
    st.set("snake/hotkey/vk", 0); st.set("snake/hotkey/mods", 0); st.set("snake/hotkey/label", QString());
    GlobalHotkeys::instance().remove(HkToggle);
    emit changed();
}

void SnakeGame::captureHotkey() {
    if (m_capturing) return;
    m_capturing = true;
    QCoreApplication::instance()->installEventFilter(this);
    emit changed();
}
void SnakeGame::cancelCapture() {
    if (!m_capturing) return;
    m_capturing = false;
    QCoreApplication::instance()->removeEventFilter(this);
    emit changed();
}

bool SnakeGame::eventFilter(QObject* o, QEvent* e) {
    if (!m_capturing) return QObject::eventFilter(o, e);
    if (e->type() == QEvent::ShortcutOverride) { e->accept(); return true; }
    if (e->type() == QEvent::KeyRelease) return true;                  // swallow while capturing
    if (e->type() != QEvent::KeyPress) return QObject::eventFilter(o, e);
    if (!o->isWindowType()) return true;                               // the window gets the event first; ignore the second delivery
    auto* k = static_cast<QKeyEvent*>(e);
    const int key = k->key();
    if (key == Qt::Key_Escape) { cancelCapture(); return true; }
    if (key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Alt || key == Qt::Key_Meta || key == Qt::Key_AltGr) return true;   // wait for the real key
    const int mods = int(k->modifiers() & (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier));
    if (mods == 0) { setMessage("Add a modifier (⌘ ⌃ ⌥ ⇧) so the shortcut does not steal a normal key"); return true; }
    cancelCapture();
    setHotkey(key, mods, k->nativeVirtualKey());
    return true;
}
}
