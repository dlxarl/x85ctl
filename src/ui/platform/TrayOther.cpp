#include "Tray.h"
namespace kcc {
bool trayAvailable() { return false; }
void installTray(const QByteArray&, const QString&, TrayActions) {}
void setTrayStatus(const QString&) {}
void setTrayProfiles(const QStringList&, const QString&, std::function<void(const QString&)>) {}
void setDockIconVisible(bool) {}
void bringAppToFront() {}
void hideApp() {}
void installCloseToHide(QWindow*, std::function<void()>) {}
}
