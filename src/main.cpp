#include "core/DeviceManager.h"
#include "services/KeyMappingService.h"
#include "services/LightingService.h"
#include "services/MacroService.h"
#include "services/KeyRecorder.h"
#include "services/SnakeGame.h"
#include "services/AppShell.h"
#include "services/ProfileService.h"
#include "protocol/X85Layout.h"
#include <cmath>
#include "ui/viewmodels/KeyboardLayoutModel.h"
#include "ui/platform/Tray.h"
#include "ui/platform/WindowChrome.h"
#include <QGuiApplication>
#include <QFile>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/KCC/assets/app-icon.png"));   // Windows/Linux taskbar; macOS uses AppIcon.icns
    app.setOrganizationName("KeyboardControlCenter");
    app.setApplicationName("Keyboard Control Center");
    app.setApplicationVersion(QStringLiteral(KCC_VERSION));
    QQuickStyle::setStyle("Basic");

    kcc::DeviceManager device;
    kcc::LightingService lighting(&device);
    kcc::KeyMappingService keymap(&device);
    kcc::MacroService macros(&device);
    keymap.setMacros(&macros);
    kcc::KeyRecorder recorder(&macros);
    kcc::SnakeGame snake(&device, &lighting);
    kcc::AppShell shell;
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &snake, &kcc::SnakeGame::stop);
    kcc::ProfileService profiles(&device, &lighting, &keymap);
    profiles.setMacros(&macros);
    kcc::KeyboardLayoutModel layout;

    QQmlApplicationEngine engine;
    auto* ctx = engine.rootContext();
    ctx->setContextProperty("Device", &device);
    ctx->setContextProperty("Lighting", &lighting);
    ctx->setContextProperty("KeyMap", &keymap);
    ctx->setContextProperty("Macros", &macros);
    ctx->setContextProperty("Recorder", &recorder);
    ctx->setContextProperty("Snake", &snake);
    ctx->setContextProperty("Shell", &shell);
    ctx->setContextProperty("ProfileStore", &profiles);
    ctx->setContextProperty("KbLayout", &layout);

    // Developer aid: KCC_SNAPSHOT_DIR=/path renders every page to PNG and exits (works offscreen).
    const QString snapDir = qEnvironmentVariable("KCC_SNAPSHOT_DIR");
    const QString snapTheme = snapDir.isEmpty() ? QString() : qEnvironmentVariable("KCC_SNAPSHOT_THEME", "light");
    ctx->setContextProperty("SnapshotTheme", snapTheme);
    if (!snapDir.isEmpty() && qEnvironmentVariableIsSet("KCC_SNAPSHOT_PERKEY")) {   // demo colours for screenshots
        for (const auto& k : kcc::x85::layout())
            lighting.setKeyColor(k.lightIndex, QColor::fromHsvF(std::fmod(k.x / 1140.0 + k.y / 2600.0, 1.0), 0.85, 1.0));
        lighting.setPerKeyActive(true);
    }

    if (!snapDir.isEmpty() && qEnvironmentVariableIsSet("KCC_SNAPSHOT_MACRO")) {   // demo macro for screenshots
        const int m = macros.add("Copy and paste");
        macros.addStep(m, "key", 0x06, 1); macros.addStep(m, "delay", 80); macros.addStep(m, "key", 0x19, 1);
        macros.addStep(macros.add("Double click"), "mouse", 1);
        keymap.assignMacro(30, 0, 1, 3);
    }
    if (!snapDir.isEmpty() && qEnvironmentVariableIsSet("KCC_SNAPSHOT_SNAKE")) {   // demo game for screenshots
        snake.seed(3); snake.beginRun();
        for (int i = 0; i < 4; ++i) { snake.placeFood(snake.neighbour(snake.body().first(), kcc::SnakeGame::Right)); snake.step(); }
        snake.turn(kcc::SnakeGame::Up); snake.step(); snake.turn(kcc::SnakeGame::Right); snake.step(); snake.step();
    }
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/KCC/Main.qml")));
    if (!engine.rootObjects().isEmpty())   // our top bar (52 px) replaces the native title bar
        kcc::applyWindowChrome(qobject_cast<QWindow*>(engine.rootObjects().first()), 52, 18);

    // Background mode: closing the window hides it; the app keeps running from the menu bar / tray.
    const bool smoke = app.arguments().contains("--kcc-smoke-test");
    if (kcc::trayAvailable() && snapDir.isEmpty() && !smoke && !engine.rootObjects().isEmpty()) {
        auto* win = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        app.setQuitOnLastWindowClosed(false);
        auto showWindow = [win] {
            kcc::setDockIconVisible(true);
            win->show(); win->raise(); win->requestActivate();
            kcc::bringAppToFront();
        };
        QFile f(":/KCC/assets/tray.png");
        const QByteArray png = f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
        // The status item only attaches once NSApplication has finished launching, i.e. inside the event loop.
        QTimer::singleShot(300, win, [png, showWindow, &device, &profiles] {
            kcc::installTray(png, "ATTACK SHARK Control Center", {showWindow, [] { QCoreApplication::quit(); }});
            auto refreshProfiles = [&profiles, &device] {
                kcc::setTrayProfiles(profiles.profiles(), profiles.active(), [&profiles, &device](const QString& n) {
                    profiles.activate(n);
                    if (device.ready() && device.writesEnabled()) profiles.applyToKeyboard();   // quick switch: load and send
                });
            };
            refreshProfiles();
            QObject::connect(&profiles, &kcc::ProfileService::changed, &profiles, refreshProfiles, Qt::QueuedConnection);   // never rebuild the menu mid-click
            kcc::setTrayStatus(device.ready() ? "Keyboard connected" : device.connected() ? "Keyboard found — config interface unavailable"
                                                                                         : "Keyboard not connected");
        });
        kcc::installCloseToHide(win, [win] { win->hide(); kcc::hideApp(); });
        if (qEnvironmentVariableIsSet("KCC_HIDE_TEST")) {   // dev aid: close-to-tray round trip without clicking
            QTimer::singleShot(3000, win, [win] { win->hide(); });
            QTimer::singleShot(7000, win, [showWindow] { showWindow(); });
        }
        // no window → no Dock icon, only the menu-bar item
        QObject::connect(win, &QWindow::visibleChanged, win, [&shell](bool v) { kcc::setDockIconVisible(shell.dockVisibleFor(v)); });
        QObject::connect(&shell, &kcc::AppShell::changed, win, [&shell, win] { kcc::setDockIconVisible(shell.dockVisibleFor(win->isVisible())); });
        // opening the app again (Launchpad / Finder) while it lives in the tray brings the window back
        QObject::connect(&app, &QGuiApplication::applicationStateChanged, win, [win, showWindow](Qt::ApplicationState st) {
            if (st == Qt::ApplicationActive && !win->isVisible()) showWindow();
        });
        auto status = [&device] {
            kcc::setTrayStatus(device.ready() ? "Keyboard connected" : device.connected() ? "Keyboard found — config interface unavailable"
                                                                                         : "Keyboard not connected");
        };
        QObject::connect(&device, &kcc::DeviceManager::changed, win, status);
    }
    if (qEnvironmentVariableIsSet("KCC_FS_TEST") && !engine.rootObjects().isEmpty()) {   // dev aid: full screen round trip
        auto* w = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        QTimer::singleShot(2500, w, [w] { w->showFullScreen(); });
        QTimer::singleShot(6500, w, [w] { w->showNormal(); });
    }
    // Packaging smoke test: all libraries, plugins and QML modules resolved → exit 0.
    if (app.arguments().contains("--kcc-smoke-test")) return engine.rootObjects().isEmpty() ? 1 : 0;

    if (!snapDir.isEmpty() && !engine.rootObjects().isEmpty()) {
        auto* win = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        if (const int h = qEnvironmentVariableIntValue("KCC_SNAPSHOT_HEIGHT"); h > 0) win->resize(win->width(), h);
        auto* t = new QTimer(&app);
        QObject::connect(t, &QTimer::timeout, &app, [=, step = 0]() mutable {
            const int n = win->property("sections").toList().size();
            if (step > 0) win->grabWindow().save(QString("%1/page%2-%3.png").arg(snapDir).arg(step - 1).arg(snapTheme));
            if (step >= n) { QCoreApplication::quit(); return; }
            win->setProperty("current", step++);
        });
        t->start(700);
    }
    return app.exec();
}
