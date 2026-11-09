#include "ProfileService.h"
#include "KeyMappingService.h"
#include "LightingService.h"
#include "MacroService.h"
#include "core/DeviceManager.h"
#include "protocol/CommandBuilder.h"
#include "storage/SettingsRepository.h"
#include <QDateTime>

namespace kcc {
ProfileService::ProfileService(DeviceManager* d, LightingService* l, KeyMappingService* k, QObject* p)
    : QObject(p), m_dev(d), m_light(l), m_keys(k) {
    // The X85 cannot report its effect settings or keymap (no read command in the vendor protocol),
    // so every edit is saved into the active profile: on the next start the app shows what was last set.
    m_autosave.setSingleShot(true);
    m_autosave.setInterval(800);
    connect(&m_autosave, &QTimer::timeout, this, [this] {
        if (!m_active.isEmpty()) m_repo.save(m_active, snapshot());
    });
    auto touched = [this] { if (!m_loading) m_autosave.start(); };
    connect(l, &LightingService::changed, this, touched);
    connect(l, &LightingService::perKeyChanged, this, touched);
    connect(k, &KeyMappingService::changed, this, touched);
    if (m_repo.names().isEmpty()) m_repo.save("Default", snapshot());
    const QString last = SettingsRepository().value("profiles/active").toString();
    activate(m_repo.exists(last) ? last : m_repo.names().value(0));
}

void ProfileService::setMacros(MacroService* m) {
    m_macros = m;
    // the constructor already activated a profile (before macros existed here): load its macros now
    m_loading = true;
    if (!m_active.isEmpty()) m->fromJson(m_repo.load(m_active)["macros"].toArray());
    m_loading = false;
    connect(m, &MacroService::changed, this, [this] { if (!m_loading) m_autosave.start(); });
}

QJsonObject ProfileService::snapshot() const {
    QJsonObject o{{"device", "x85"}, {"updated", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
                  {"lighting", m_light->toJson()}, {"keymap", m_keys->toJson()}};
    if (m_macros) o["macros"] = m_macros->toJson();
    return o;
}

bool ProfileService::create(const QString& name) {
    const QString n = ProfileRepository::sanitize(name);
    if (n.isEmpty() || m_repo.exists(n)) { say("Name is empty or already used"); return false; }
    const bool ok = m_repo.save(n, snapshot());
    emit changed();
    if (ok) activate(n);
    return ok;
}
bool ProfileService::activate(const QString& n) {
    const auto o = m_repo.load(n);
    if (o.isEmpty()) return false;
    if (m_autosave.isActive()) { m_autosave.stop(); if (!m_active.isEmpty()) m_repo.save(m_active, snapshot()); }
    m_loading = true;
    m_light->fromJson(o["lighting"].toObject());
    if (m_macros) m_macros->fromJson(o["macros"].toArray());   // before the keymap: labels need the names
    m_keys->fromJson(o["keymap"].toObject());
    m_loading = false;
    m_active = ProfileRepository::sanitize(n);
    SettingsRepository().set("profiles/active", m_active);
    emit changed();
    return true;
}
bool ProfileService::saveActive() {
    const bool ok = !m_active.isEmpty() && m_repo.save(m_active, snapshot());
    say(ok ? "Saved “" + m_active + "”" : "Save failed");
    return ok;
}
bool ProfileService::overwrite(const QString& n) {
    if (!m_repo.exists(n)) { say("No such profile"); return false; }
    const bool ok = m_repo.save(n, snapshot());
    say(ok ? "Overwrote “" + n + "” with the current settings" : "Save failed");
    return ok;
}
bool ProfileService::remove(const QString& n) {
    if (m_repo.names().size() <= 1) return false;
    const bool ok = m_repo.remove(n);
    if (n == m_active) activate(m_repo.names().value(0));
    emit changed();
    return ok;
}
bool ProfileService::rename(const QString& from, const QString& to) {
    const QString t = ProfileRepository::sanitize(to);
    if (t.isEmpty() || m_repo.exists(t)) return false;
    const auto o = m_repo.load(from);
    if (o.isEmpty() || !m_repo.save(t, o)) return false;
    m_repo.remove(from);
    if (from == m_active) { m_active = t; SettingsRepository().set("profiles/active", t); }
    emit changed();
    return true;
}
bool ProfileService::exportProfile(const QString& n, const QUrl& f) {
    const bool ok = m_repo.exportTo(n, f.toLocalFile());
    say(ok ? "Exported to " + f.toLocalFile() : "Export failed");
    return ok;
}
bool ProfileService::importProfile(const QUrl& f) {
    QString err;
    const QString n = m_repo.importFrom(f.toLocalFile(), &err);
    emit changed();
    say(n.isEmpty() ? "Import failed: " + err : "Imported “" + n + "”");
    return !n.isEmpty();
}
bool ProfileService::applyToKeyboard() {
    saveActive();
    // Lighting (effect, or the per-key table when that is active) + both keymap layers.
    bool ok = m_light->applyCurrent();
    if (m_macros && m_keys->usesMacros()) ok = ok && m_macros->apply();   // bindings point into the macro table
    ok = ok && m_dev->submit(CommandBuilder::keymap(false, m_keys->layerMap(0)), "Profile: keymap base", "keymap0");
    ok = ok && m_dev->submit(CommandBuilder::keymap(true, m_keys->layerMap(1)), "Profile: keymap Fn", "keymap1");
    say(ok ? "Sending profile to keyboard…" : "Not sent: enable device writes in Device Info");
    return ok;
}
}
