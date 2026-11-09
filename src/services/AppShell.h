#pragma once
#include <QObject>

namespace kcc {
// Background-mode preferences. The menu-bar icon can be hidden by menu-bar managers or a crowded bar, so by
// default the Dock icon is hidden while the window is closed; the menu-bar icon (or reopening the app) brings it back.
class AppShell : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool hideDockInBackground READ hideDock WRITE setHideDock NOTIFY changed)
    Q_PROPERTY(bool trayAvailable READ trayAvailable CONSTANT)
public:
    explicit AppShell(QObject* parent = nullptr);
    bool hideDock() const { return m_hideDock; }
    void setHideDock(bool on);
    static bool trayAvailable();
    bool dockVisibleFor(bool windowVisible) const { return windowVisible || !m_hideDock; }
signals:
    void changed();
private:
    bool m_hideDock = true;
};
}
