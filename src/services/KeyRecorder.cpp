#include "KeyRecorder.h"
#include "MacroService.h"
#include <QCoreApplication>
#include <QKeyEvent>

namespace kcc {

// macOS virtual key codes (Carbon kVK_*) → HID keyboard usages.
static int hidFromMac(quint32 vk) {
    static const QHash<quint32, int> t = {
        {0x00, 0x04}, {0x01, 0x16}, {0x02, 0x07}, {0x03, 0x09}, {0x04, 0x0B}, {0x05, 0x0A}, {0x06, 0x1D}, {0x07, 0x1B},
        {0x08, 0x06}, {0x09, 0x19}, {0x0A, 0x64}, {0x0B, 0x05}, {0x0C, 0x14}, {0x0D, 0x1A}, {0x0E, 0x08}, {0x0F, 0x15},
        {0x10, 0x1C}, {0x11, 0x17}, {0x12, 0x1E}, {0x13, 0x1F}, {0x14, 0x20}, {0x15, 0x21}, {0x16, 0x23}, {0x17, 0x22},
        {0x18, 0x2E}, {0x19, 0x26}, {0x1A, 0x24}, {0x1B, 0x2D}, {0x1C, 0x25}, {0x1D, 0x27}, {0x1E, 0x30}, {0x1F, 0x12},
        {0x20, 0x18}, {0x21, 0x2F}, {0x22, 0x0C}, {0x23, 0x13}, {0x24, 0x28}, {0x25, 0x0F}, {0x26, 0x0D}, {0x27, 0x34},
        {0x28, 0x0E}, {0x29, 0x33}, {0x2A, 0x31}, {0x2B, 0x36}, {0x2C, 0x38}, {0x2D, 0x11}, {0x2E, 0x10}, {0x2F, 0x37},
        {0x30, 0x2B}, {0x31, 0x2C}, {0x32, 0x35}, {0x33, 0x2A}, {0x35, 0x29},
        {0x36, 0xE7}, {0x37, 0xE3}, {0x38, 0xE1}, {0x39, 0x39}, {0x3A, 0xE2}, {0x3B, 0xE0}, {0x3C, 0xE5}, {0x3D, 0xE6}, {0x3E, 0xE4},
        {0x40, 0x6C}, {0x41, 0x63}, {0x43, 0x55}, {0x45, 0x57}, {0x47, 0x53}, {0x4B, 0x54}, {0x4C, 0x58}, {0x4E, 0x56},
        {0x4F, 0x6D}, {0x50, 0x6E}, {0x51, 0x67}, {0x52, 0x62}, {0x53, 0x59}, {0x54, 0x5A}, {0x55, 0x5B}, {0x56, 0x5C},
        {0x57, 0x5D}, {0x58, 0x5E}, {0x59, 0x5F}, {0x5A, 0x6F}, {0x5B, 0x60}, {0x5C, 0x61},
        {0x60, 0x3E}, {0x61, 0x3F}, {0x62, 0x40}, {0x63, 0x3C}, {0x64, 0x41}, {0x65, 0x42}, {0x67, 0x44}, {0x69, 0x68},
        {0x6A, 0x6B}, {0x6B, 0x69}, {0x6D, 0x43}, {0x6F, 0x45}, {0x71, 0x6A}, {0x72, 0x49}, {0x73, 0x4A}, {0x74, 0x4B},
        {0x75, 0x4C}, {0x76, 0x3D}, {0x77, 0x4D}, {0x78, 0x3B}, {0x79, 0x4E}, {0x7A, 0x3A}, {0x7B, 0x50}, {0x7C, 0x4F},
        {0x7D, 0x51}, {0x7E, 0x52},
    };
    return t.value(vk, 0);
}

// Portable fallback by Qt key (no left/right modifier distinction).
static int hidFromQtKey(int k) {
    if (k >= Qt::Key_A && k <= Qt::Key_Z) return 0x04 + (k - Qt::Key_A);
    if (k >= Qt::Key_1 && k <= Qt::Key_9) return 0x1E + (k - Qt::Key_1);
    if (k == Qt::Key_0) return 0x27;
    if (k >= Qt::Key_F1 && k <= Qt::Key_F12) return 0x3A + (k - Qt::Key_F1);
    if (k >= Qt::Key_F13 && k <= Qt::Key_F24) return 0x68 + (k - Qt::Key_F13);
    static const QHash<int, int> t = {
        {Qt::Key_Return, 0x28}, {Qt::Key_Enter, 0x58}, {Qt::Key_Escape, 0x29}, {Qt::Key_Backspace, 0x2A}, {Qt::Key_Tab, 0x2B},
        {Qt::Key_Backtab, 0x2B}, {Qt::Key_Space, 0x2C}, {Qt::Key_Minus, 0x2D}, {Qt::Key_Underscore, 0x2D}, {Qt::Key_Equal, 0x2E},
        {Qt::Key_Plus, 0x2E}, {Qt::Key_BracketLeft, 0x2F}, {Qt::Key_BraceLeft, 0x2F}, {Qt::Key_BracketRight, 0x30},
        {Qt::Key_BraceRight, 0x30}, {Qt::Key_Backslash, 0x31}, {Qt::Key_Bar, 0x31}, {Qt::Key_Semicolon, 0x33}, {Qt::Key_Colon, 0x33},
        {Qt::Key_Apostrophe, 0x34}, {Qt::Key_QuoteDbl, 0x34}, {Qt::Key_QuoteLeft, 0x35}, {Qt::Key_AsciiTilde, 0x35},
        {Qt::Key_Comma, 0x36}, {Qt::Key_Less, 0x36}, {Qt::Key_Period, 0x37}, {Qt::Key_Greater, 0x37}, {Qt::Key_Slash, 0x38},
        {Qt::Key_Question, 0x38}, {Qt::Key_CapsLock, 0x39}, {Qt::Key_Print, 0x46}, {Qt::Key_ScrollLock, 0x47}, {Qt::Key_Pause, 0x48},
        {Qt::Key_Insert, 0x49}, {Qt::Key_Home, 0x4A}, {Qt::Key_PageUp, 0x4B}, {Qt::Key_Delete, 0x4C}, {Qt::Key_End, 0x4D},
        {Qt::Key_PageDown, 0x4E}, {Qt::Key_Right, 0x4F}, {Qt::Key_Left, 0x50}, {Qt::Key_Down, 0x51}, {Qt::Key_Up, 0x52},
        {Qt::Key_NumLock, 0x53}, {Qt::Key_Menu, 0x65},
        {Qt::Key_Exclam, 0x1E}, {Qt::Key_At, 0x1F}, {Qt::Key_NumberSign, 0x20}, {Qt::Key_Dollar, 0x21}, {Qt::Key_Percent, 0x22},
        {Qt::Key_AsciiCircum, 0x23}, {Qt::Key_Ampersand, 0x24}, {Qt::Key_Asterisk, 0x25}, {Qt::Key_ParenLeft, 0x26}, {Qt::Key_ParenRight, 0x27},
#ifdef Q_OS_MACOS   // Qt swaps them on macOS: Key_Control is ⌘, Key_Meta is ⌃
        {Qt::Key_Control, 0xE3}, {Qt::Key_Meta, 0xE0},
#else
        {Qt::Key_Control, 0xE0}, {Qt::Key_Meta, 0xE3},
#endif
        {Qt::Key_Shift, 0xE1}, {Qt::Key_Alt, 0xE2}, {Qt::Key_AltGr, 0xE6},
    };
    return t.value(k, 0);
}

int KeyRecorder::hidFromEvent(const QKeyEvent* e) {
#ifdef Q_OS_MACOS
    // kVK_ANSI_A is 0, which is also what an event without a native code carries: trust 0 only for A.
    if (const quint32 vk = e->nativeVirtualKey(); vk != 0 || e->key() == Qt::Key_A)
        if (const int h = hidFromMac(vk)) return h;
#endif
    return hidFromQtKey(e->key());
}

KeyRecorder::KeyRecorder(MacroService* m, QObject* p) : QObject(p), m_macros(m) {
    connect(m, &MacroService::removed, this, [this](int idx) {
        if (idx == m_macro) stop();
        else if (idx < m_macro) --m_macro;
    });
}

void KeyRecorder::start(int macroIndex, int at) {
    if (macroIndex < 0 || macroIndex >= m_macros->list().size()) return;
    stop();
    m_macro = macroIndex; m_at = at; m_count = 0; m_down.clear();
    m_clock.invalidate();
    QCoreApplication::instance()->installEventFilter(this);
    emit changed();
}

void KeyRecorder::stop() {
    if (m_macro < 0) return;
    QCoreApplication::instance()->removeEventFilter(this);
    // keys still held when recording stops: release them so the macro never leaves a key stuck down
    for (int u : std::as_const(m_down)) {
        const int n = m_macros->addStep(m_macro, "keyup", u, 0, m_at);
        if (m_at >= 0) m_at += n;
        m_count += n;
    }
    m_down.clear();
    m_macro = -1;
    emit changed();
}

bool KeyRecorder::eventFilter(QObject* o, QEvent* e) {
    const auto t = e->type();
    if (t == QEvent::ShortcutOverride) { e->accept(); return true; }    // no app shortcuts while recording
    if (t != QEvent::KeyPress && t != QEvent::KeyRelease) return QObject::eventFilter(o, e);
    auto* k = static_cast<QKeyEvent*>(e);
    // The same key event is delivered to the window and then to the focus item: handle it once.
    if (!o->isWindowType()) return true;
    if (k->isAutoRepeat()) return true;
    const int usage = hidFromEvent(k);
    if (!usage) return true;
    const bool down = t == QEvent::KeyPress;
    if (down == m_down.contains(usage)) return true;                     // duplicate (e.g. lost release)
    if (m_macros->bytesUsed() + 8 > m_macros->bytesMax()) { stop(); return true; }   // memory full
    auto add = [&](const QString& type, int v) {
        const int n = m_macros->addStep(m_macro, type, v, 0, m_at);
        if (m_at >= 0) m_at += n;
        m_count += n;
    };
    if (m_delays && m_clock.isValid()) {
        const qint64 ms = m_clock.elapsed();
        if (ms >= 10) add("delay", int(qMin<qint64>(ms, 0xFFFF)));
    }
    m_clock.restart();
    add(down ? "keydown" : "keyup", usage);
    if (down) m_down.insert(usage); else m_down.remove(usage);
    emit changed();
    return true;
}
}
