#pragma once
#include <QObject>

namespace kcc {
// System-wide hotkeys (work while another app is in front). macOS: Carbon RegisterEventHotKey, which needs
// no Accessibility / Input Monitoring permission. Other platforms: not implemented yet (add() returns false).
// Keys are macOS virtual key codes and Carbon modifier masks; see modsFromQt().
class GlobalHotkeys : public QObject {
    Q_OBJECT
public:
    static GlobalHotkeys& instance();
    bool add(int id, quint32 nativeVk, quint32 carbonMods);   // replaces an existing id
    void remove(int id);
    bool has(int id) const;
    static bool supported();
    static quint32 modsFromQt(int qtModifiers);   // Qt's macOS swap: ControlModifier is ⌘, MetaModifier is ⌃
signals:
    void activated(int id);
private:
    GlobalHotkeys() = default;
};
}
