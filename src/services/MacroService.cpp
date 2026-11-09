#include "MacroService.h"
#include "KeyCatalog.h"
#include "core/DeviceManager.h"
#include <QJsonObject>

namespace kcc {
using T = MacroEvent::Type;

static QString label(const MacroEvent& e) {
    switch (e.type) {
    case T::Delay:     return QString("Wait %1 ms").arg(e.value);
    case T::KeyDown:   return "Press " + KeyCatalog::hidName(e.value);
    case T::KeyUp:     return "Release " + KeyCatalog::hidName(e.value);
    case T::MouseDown: return "Press " + KeyCatalog::mouseName(uint8_t(e.value));
    case T::MouseUp:   return "Release " + KeyCatalog::mouseName(uint8_t(e.value));
    }
    return {};
}

QVariantList MacroService::macros() const {
    QVariantList l;
    for (const auto& m : m_list) l << QVariantMap{{"name", m.name}, {"steps", int(m.events.size())}, {"empty", m.events.isEmpty()}};
    return l;
}

QString MacroService::error() const {
    QString e;
    if (!m_list.isEmpty()) CommandBuilder::macros(m_list, &e);
    return e;
}

int MacroService::add(const QString& name) {
    if (m_list.size() >= CommandBuilder::kMacroMaxCount) return -1;
    MacroDef m; m.name = name.trimmed().isEmpty() ? QString("Macro %1").arg(m_list.size() + 1) : name.trimmed();
    m_list << m; emit changed();
    return int(m_list.size()) - 1;
}
void MacroService::remove(int i) {
    if (i < 0 || i >= m_list.size()) return;
    m_list.removeAt(i);
    emit removed(i); emit changed();
}
void MacroService::rename(int i, const QString& n) {
    if (i < 0 || i >= m_list.size() || n.trimmed().isEmpty() || m_list[i].name == n.trimmed()) return;
    m_list[i].name = n.trimmed(); emit changed();
}
QVariantList MacroService::steps(int i) const {
    QVariantList l;
    if (i < 0 || i >= m_list.size()) return l;
    for (const auto& e : m_list[i].events) l << QVariantMap{{"type", int(e.type)}, {"value", e.value}, {"label", label(e)}};
    return l;
}

int MacroService::addStep(int i, const QString& type, int value, int mods, int at) {
    if (i < 0 || i >= m_list.size()) return 0;
    QVector<MacroEvent> ev;
    auto push = [&](T t, int v) { ev << MacroEvent{t, uint16_t(v)}; };
    if (type == "delay") push(T::Delay, qBound(10, value, 0xFFFF));
    else if (type == "key" || type == "keydown" || type == "keyup") {
        static const int modUsage[4] = {0xE0, 0xE1, 0xE2, 0xE3};   // Ctrl, Shift, Alt, Win
        if (type != "keyup") for (int b = 0; b < 4; ++b) if (mods & (1 << b)) push(T::KeyDown, modUsage[b]);
        if (type != "keyup") push(T::KeyDown, value & 0xFF);
        if (type == "key" || type == "keyup") push(T::KeyUp, value & 0xFF);
        if (type == "key") for (int b = 3; b >= 0; --b) if (mods & (1 << b)) push(T::KeyUp, modUsage[b]);
    } else if (type == "mouse") { push(T::MouseDown, value & 7); push(T::MouseUp, value & 7); }
    else if (type == "mousedown") push(T::MouseDown, value & 7);
    else if (type == "mouseup") push(T::MouseUp, value & 7);
    else return 0;
    auto& dst = m_list[i].events;
    const int pos = (at < 0 || at > dst.size()) ? int(dst.size()) : at;
    for (int k = 0; k < ev.size(); ++k) dst.insert(pos + k, ev[k]);
    emit changed();
    return int(ev.size());
}
void MacroService::setStepValue(int i, int s, int v) {
    if (i < 0 || i >= m_list.size() || s < 0 || s >= m_list[i].events.size()) return;
    auto& e = m_list[i].events[s];
    const int nv = e.type == T::Delay ? qBound(10, v, 0xFFFF) : (e.type == T::KeyDown || e.type == T::KeyUp) ? (v & 0xFF) : (v & 7);
    if (e.value == nv) return;
    e.value = uint16_t(nv); emit changed();
}
void MacroService::removeStep(int i, int s) {
    if (i < 0 || i >= m_list.size() || s < 0 || s >= m_list[i].events.size()) return;
    m_list[i].events.removeAt(s); emit changed();
}
void MacroService::moveStep(int i, int s, int d) {
    if (i < 0 || i >= m_list.size()) return;
    auto& ev = m_list[i].events; const int t = s + d;
    if (s < 0 || s >= ev.size() || t < 0 || t >= ev.size()) return;
    ev.swapItemsAt(s, t); emit changed();
}
void MacroService::clearSteps(int i) { if (i >= 0 && i < m_list.size() && !m_list[i].events.isEmpty()) { m_list[i].events.clear(); emit changed(); } }

void MacroService::setAllDelays(int i, int ms) {
    if (i < 0 || i >= m_list.size()) return;
    auto& ev = m_list[i].events;
    if (ms <= 0) ev.removeIf([](const MacroEvent& e) { return e.type == T::Delay; });
    else for (auto& e : ev) if (e.type == T::Delay) e.value = uint16_t(qBound(10, ms, 0xFFFF));
    emit changed();
}
int MacroService::totalMs(int i) const {
    int t = 0;
    if (i >= 0 && i < m_list.size()) for (const auto& e : m_list[i].events) if (e.type == T::Delay) t += e.value;
    return t;
}

bool MacroService::apply() {
    QString err;
    const auto tx = CommandBuilder::macros(m_list, &err);
    if (tx.isEmpty()) return false;
    return m_dev->submit(tx, "Macros", "macros");
}

QJsonArray MacroService::toJson() const {
    QJsonArray a;
    for (const auto& m : m_list) {
        QJsonArray ev;
        for (const auto& e : m.events) ev << QJsonObject{{"t", int(e.type)}, {"v", int(e.value)}};
        a << QJsonObject{{"name", m.name}, {"events", ev}};
    }
    return a;
}
void MacroService::fromJson(const QJsonArray& a) {
    m_list.clear();
    for (const auto& v : a) {
        if (m_list.size() >= CommandBuilder::kMacroMaxCount) break;
        const auto o = v.toObject();
        MacroDef m; m.name = o["name"].toString("Macro");
        for (const auto& ev : o["events"].toArray()) {
            const auto eo = ev.toObject(); const int t = eo["t"].toInt(-1);
            if (t < 0 || t > int(T::MouseUp)) continue;
            m.events << MacroEvent{T(t), uint16_t(eo["v"].toInt())};
        }
        m_list << m;
    }
    emit changed();
}
}
