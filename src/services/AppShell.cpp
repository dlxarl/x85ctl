#include "AppShell.h"
#include "storage/SettingsRepository.h"
#include "ui/platform/Tray.h"

namespace kcc {
AppShell::AppShell(QObject* p) : QObject(p) { m_hideDock = SettingsRepository().value("app/hideDockInBackground", true).toBool(); }
bool AppShell::trayAvailable() { return kcc::trayAvailable(); }
void AppShell::setHideDock(bool on) {
    if (on == m_hideDock) return;
    m_hideDock = on;
    SettingsRepository().set("app/hideDockInBackground", on);
    emit changed();
}
}
