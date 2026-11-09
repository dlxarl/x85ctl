#pragma once
#include "storage/ProfileRepository.h"
#include <QObject>
#include <QTimer>
#include <QUrl>

namespace kcc {
class LightingService;
class KeyMappingService;
class MacroService;
class DeviceManager;

// Profiles are stored on the host. "Apply to keyboard" uploads lighting + per-key + both keymap layers.
class ProfileService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList profiles READ profiles NOTIFY changed)
    Q_PROPERTY(QString active READ active NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
public:
    ProfileService(DeviceManager* d, LightingService* l, KeyMappingService* k, QObject* parent = nullptr);
    void setMacros(MacroService* m);
    QStringList profiles() const { return m_repo.names(); }
    QString active() const { return m_active; }
    QString message() const { return m_msg; }

    Q_INVOKABLE bool create(const QString& name);
    Q_INVOKABLE bool activate(const QString& name);   // loads into the editors
    Q_INVOKABLE bool saveActive();
    Q_INVOKABLE bool overwrite(const QString& name);   // replace any saved profile with the current settings
    Q_INVOKABLE bool remove(const QString& name);
    Q_INVOKABLE bool rename(const QString& from, const QString& to);
    Q_INVOKABLE bool exportProfile(const QString& name, const QUrl& file);
    Q_INVOKABLE bool importProfile(const QUrl& file);
    Q_INVOKABLE bool applyToKeyboard();
signals:
    void changed();
    void messageChanged();
private:
    QJsonObject snapshot() const;
    MacroService* m_macros = nullptr;
    QTimer m_autosave;
    bool m_loading = false;
    void say(const QString& m) { m_msg = m; emit messageChanged(); }
    ProfileRepository m_repo;
    DeviceManager* m_dev;
    LightingService* m_light;
    KeyMappingService* m_keys;
    QString m_active, m_msg;
};
}
