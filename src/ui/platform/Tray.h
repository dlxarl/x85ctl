#pragma once
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <functional>
class QWindow;

namespace kcc {
// Menu-bar / system-tray presence so the app keeps working with its window closed.
// macOS: a native status item (template icon, so it follows light/dark menu bars).
// Other platforms: not implemented yet — trayAvailable() is false and closing the window quits as usual.
struct TrayActions { std::function<void()> open, quit; };
bool trayAvailable();
void installTray(const QByteArray& iconPng, const QString& title, TrayActions actions);
void setTrayStatus(const QString& line);
// Profile list in the menu (checkmark on the active one); onPick gets the chosen name.
void setTrayProfiles(const QStringList& names, const QString& active, std::function<void(const QString&)> onPick);       // disabled info line at the top of the menu
// macOS: with no window shown the app drops its Dock icon and lives in the menu bar only.
void setDockIconVisible(bool visible);
void bringAppToFront();
void hideApp();                                // macOS: hide + hand focus back, so a Dock click re-activates us
// macOS: the red close button runs `hide` instead of closing (Cmd+Q still quits normally).
void installCloseToHide(QWindow* window, std::function<void()> hide);
}
