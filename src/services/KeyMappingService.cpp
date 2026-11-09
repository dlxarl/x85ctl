#include "KeyMappingService.h"
#include "KeyCatalog.h"
#include "MacroService.h"
#include "core/DeviceManager.h"
#include "protocol/X85Layout.h"

namespace kcc {
KeyMappingService::KeyMappingService(DeviceManager* d, QObject* p) : QObject(p), m_dev(d) {}

QVariantList KeyMappingService::keyTargets() const { return KeyCatalog::keys(); }
QVariantList KeyMappingService::mediaTargets() const { return KeyCatalog::consumer(); }
QVariantList KeyMappingService::mouseTargets() const { return KeyCatalog::mouse(); }

static bool validIndex(int i) {
    for (const auto& k : x85::layout()) if (k.keyIndex == i) return k.hidUsage != 0xAF;  // Fn is not remappable
    return false;
}

QString KeyMappingService::describeStatic(const KeyAction& a) {
    using K = KeyAction::Kind;
    switch (a.kind) {
    case K::Default: return {};
    case K::Key: {
        QStringList m;
        if (a.modifiers & 0x11) m << "Ctrl";
        if (a.modifiers & 0x22) m << "Shift";
        if (a.modifiers & 0x44) m << "Alt";
        if (a.modifiers & 0x88) m << "Win";
        if (a.usage) m << KeyCatalog::hidName(a.usage);
        return m.join('+');
    }
    case K::Consumer: return KeyCatalog::consumerName(a.usage);
    case K::Mouse: return KeyCatalog::mouseName(a.button);
    case K::Wheel: return a.button == 0x01 ? "Wheel up" : "Wheel down";
    case K::Macro: return QString("Macro %1").arg(a.macroIndex + 1);
    }
    return {};
}
QString KeyMappingService::describe(const KeyAction& a) const {
    if (a.kind == KeyAction::Kind::Macro && m_macros) {
        static const char* mode[] = {"once", "repeat", "toggle", "while held"};
        const QString n = m_macros->nameOf(a.macroIndex);
        QString t = n.isEmpty() ? QString("Macro %1 (missing)").arg(a.macroIndex + 1) : n;
        t += QString(" · ") + mode[a.macroMode & 3];
        if (a.macroMode == PlayRepeat) t += QString(" ×%1").arg(a.macroCount);
        return t;
    }
    return describeStatic(a);
}
QString KeyMappingService::describe(int i) const { return describe(m_map[m_layer].value(i)); }
bool KeyMappingService::usesMacros() const {
    for (int l = 0; l < 2; ++l) for (const auto& a : m_map[l]) if (a.kind == KeyAction::Kind::Macro) return true;
    return false;
}
void KeyMappingService::setMacros(MacroService* m) {
    m_macros = m;
    if (!m) return;
    connect(m, &MacroService::changed, this, [this] { emit changed(); });   // names in labels
    connect(m, &MacroService::removed, this, [this](int idx) {            // keep bindings pointing at the right macro
        for (int l = 0; l < 2; ++l)
            for (auto it = m_map[l].begin(); it != m_map[l].end();) {
                if (it->kind == KeyAction::Kind::Macro) {
                    if (it->macroIndex == idx) { it = m_map[l].erase(it); continue; }
                    if (it->macroIndex > idx) --it->macroIndex;
                }
                ++it;
            }
    });
}

QVariantMap KeyMappingService::labels() const {
    QVariantMap m;
    for (auto it = m_map[m_layer].begin(); it != m_map[m_layer].end(); ++it)   // macros get a short badge (keycaps are small)
        m.insert(QString::number(it.key()), it->kind == KeyAction::Kind::Macro ? QString("▶ %1").arg(it->macroIndex + 1) : describe(it.value()));
    return m;
}

void KeyMappingService::set(int i, const KeyAction& a) {
    if (!validIndex(i)) return;
    if (a.kind == KeyAction::Kind::Default) m_map[m_layer].remove(i); else m_map[m_layer][i] = a;
    emit changed();
}
void KeyMappingService::assignKey(int i, int usage, int mods) {
    KeyAction a; a.kind = KeyAction::Kind::Key; a.usage = uint16_t(usage & 0xFF); a.modifiers = uint8_t(mods);
    set(i, a);
}
void KeyMappingService::assignMedia(int i, int usage) {
    KeyAction a; a.kind = KeyAction::Kind::Consumer; a.usage = uint16_t(usage); set(i, a);
}
void KeyMappingService::assignMouse(int i, const QString& kind, int v) {
    KeyAction a; a.kind = kind == "wheel" ? KeyAction::Kind::Wheel : KeyAction::Kind::Mouse; a.button = uint8_t(v);
    set(i, a);
}
void KeyMappingService::assignMacro(int i, int idx, int mode, int count) {
    if (idx < 0 || idx > 99) return;
    KeyAction a; a.kind = KeyAction::Kind::Macro; a.macroIndex = uint8_t(idx);
    a.macroMode = uint8_t(qBound(0, mode, 3));
    a.macroCount = a.macroMode == PlayRepeat ? uint8_t(qBound(1, count, 255)) : 0;
    set(i, a);
}
void KeyMappingService::reset(int i) { set(i, {}); }
void KeyMappingService::resetLayer() { m_map[m_layer].clear(); emit changed(); }

bool KeyMappingService::apply() {
    // Bindings point into the macro table, so it goes to the keyboard first (same queue, in order).
    if (m_macros && usesMacros() && !m_macros->apply()) return false;
    return m_dev->submit(CommandBuilder::keymap(m_layer == 1, m_map[m_layer]),
                         m_layer ? "Keymap (Fn layer)" : "Keymap (base layer)", m_layer ? "keymap1" : "keymap0");
}

QJsonObject KeyMappingService::toJson() const {
    QJsonObject out;
    for (int l = 0; l < 2; ++l) {
        QJsonObject layer;
        for (auto it = m_map[l].begin(); it != m_map[l].end(); ++it)
            layer[QString::number(it.key())] = QString::fromLatin1(it->encode().toHex());
        out[l ? "fn" : "base"] = layer;
    }
    return out;
}
void KeyMappingService::fromJson(const QJsonObject& o) {
    for (int l = 0; l < 2; ++l) {
        m_map[l].clear();
        const auto layer = o[l ? "fn" : "base"].toObject();
        for (auto it = layer.begin(); it != layer.end(); ++it) {
            const int i = it.key().toInt();
            const auto a = KeyAction::decode(QByteArray::fromHex(it.value().toString().toLatin1()));
            if (validIndex(i) && a.kind != KeyAction::Kind::Default) m_map[l][i] = a;
        }
    }
    emit changed();
}
}
