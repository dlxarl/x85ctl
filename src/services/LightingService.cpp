#include "LightingService.h"
#include "core/DeviceManager.h"
#include "protocol/X85Layout.h"
#include "storage/SettingsRepository.h"
#include <QJsonArray>
#include <QTime>

namespace kcc {
using namespace x85;

static QByteArray bytesOf(const Transaction& tx) {
    QByteArray b;
    for (const auto& p : tx) b += p.data;
    return b;
}

LightingService::LightingService(DeviceManager* dev, QObject* p) : QObject(p), m_dev(dev) {
    m_live.setSingleShot(true);
    m_live.setInterval(250);   // debounce: one upload after the user pauses, not one per mouse move
    connect(&m_live, &QTimer::timeout, this, &LightingService::liveApply);

    SettingsRepository st;
    m_cc.enabled = st.value("lighting/correction/enabled", m_cc.enabled).toBool();
    // v2: the old default gamma 2.2 flattened gradients; drop a stored value from before the change.
    if (st.value("lighting/correction/version", 1).toInt() >= 2)
        m_cc.gamma = st.value("lighting/correction/gamma", m_cc.gamma).toDouble();
    else
        st.set("lighting/correction/version", 2);
    m_cc.gainR = st.value("lighting/correction/gainR", m_cc.gainR).toDouble();
    m_cc.gainG = st.value("lighting/correction/gainG", m_cc.gainG).toDouble();
    m_cc.gainB = st.value("lighting/correction/gainB", m_cc.gainB).toDouble();

    connect(dev, &DeviceManager::perKeyColorsRead, this, [this](const QVariantMap& m, const QString& e) {
        if (!e.isEmpty()) { m_sync = "Could not read colours from the keyboard: " + e; emit syncChanged(); return; }
        m_sync = QString("Per-key colours read from the keyboard (%1 keys) at %2")
                     .arg(m.size()).arg(QTime::currentTime().toString("HH:mm"));
        emit syncChanged();
        // The board holds corrected PWM values; undo the correction for display.
        m_device.clear();
        for (auto it = m.begin(); it != m.end(); ++it) {
            const QColor c = it.value().value<QColor>();
            m_device[it.key().toInt()] = m_cc.invert(Rgb{uint8_t(c.red()), uint8_t(c.green()), uint8_t(c.blue())});
        }
        m_deviceShown = !m_device.isEmpty();
        // A connect-time read must not overwrite a preset (and during an effect it is only a frozen animation frame);
        // only the Read button imports it.
        if (m_importRead) {
            m_perKey = m_device; m_importRead = false;
            emit perKeyChanged();
        }
        emit syncChanged();
    });
    // Whatever we assumed about the board's state is void after a failure or a reconnect.
    connect(dev, &DeviceManager::transactionFinished, this, [this](const QString&, bool ok, const QString&) {
        if (!ok) { m_boardInUserMode = false; m_lastSent.clear(); }
    });
    connect(dev, &DeviceManager::changed, this, [this] {
        const bool ready = m_dev->ready();
        if (!ready) { m_boardInUserMode = false; m_lastSent.clear(); }
        // The only state the X85 can report is its per-key colour table (04 F5): read it on connect.
        if (ready && !m_wasReady && !qEnvironmentVariableIsSet("KCC_SNAPSHOT_DIR"))   // never touch hardware from snapshots
            QTimer::singleShot(400, this, [this] { if (m_dev->ready()) m_dev->readPerKeyColors(); });
        m_wasReady = ready;
    });
}

QVariantList LightingService::modes() const {
    QVariantList l;
    for (const auto& m : kModes) l << QVariantMap{{"id", m.id}, {"name", m.name}, {"caps", m.caps}};
    return l;
}
int LightingService::caps() const { const auto* m = findMode(m_s.mode); return m ? m->caps : 0; }

// ---- editing --------------------------------------------------------------
void LightingService::touched() {
    if (m_deviceShown) { m_deviceShown = false; emit syncChanged(); }
    emit changed(); scheduleLive();
}
void LightingService::scheduleLive() { if (m_auto && !m_stroke) m_live.start(); }
void LightingService::setAutoApply(bool on) {
    if (on == m_auto) return;
    m_auto = on; emit changed();
    if (on) scheduleLive();
}
void LightingService::setPerKeyActive(bool on) {
    if (on == m_perKeyActive) return;
    m_perKeyActive = on; emit changed(); scheduleLive();
}
void LightingService::setMode(int v) { if (findMode(uint8_t(v)) && v != m_s.mode) { m_s.mode = uint8_t(v); touched(); } }
void LightingService::setBrightness(int v) { v = qBound(0, v, kBrightnessMax); if (v != m_s.brightness) { m_s.brightness = uint8_t(v); touched(); } }
void LightingService::setSpeed(int v) { v = qBound(0, v, kSpeedMax); if (v != m_s.speed) { m_s.speed = uint8_t(v); touched(); } }
void LightingService::setDirection(int v) { v = qBound(0, v, 3); if (v != int(m_s.direction)) { m_s.direction = Direction(v); touched(); } }
void LightingService::setColorful(bool v) { if (v != m_s.colorful) { m_s.colorful = v; touched(); } }
void LightingService::setColor(const QColor& c) {
    const Rgb n{uint8_t(c.red()), uint8_t(c.green()), uint8_t(c.blue())};
    if (n == Rgb{m_s.r, m_s.g, m_s.b}) return;
    m_s.r = n.r; m_s.g = n.g; m_s.b = n.b; touched();
}

QVariantMap LightingService::perKey() const {
    QVariantMap out;
    for (auto it = m_perKey.begin(); it != m_perKey.end(); ++it)
        out.insert(QString::number(it.key()), QColor(it->r, it->g, it->b));
    return out;
}
void LightingService::setKeyColor(int i, const QColor& c) {
    const Rgb n{uint8_t(c.red()), uint8_t(c.green()), uint8_t(c.blue())};
    if (m_perKey.contains(i) && m_perKey[i] == n) return;
    m_perKey[i] = n;
    editedPerKey();
    emit perKeyChanged();
    if (m_perKeyActive) scheduleLive();
}
void LightingService::clearKey(int i) {
    if (m_perKey.remove(i)) { editedPerKey(); emit perKeyChanged(); if (m_perKeyActive) scheduleLive(); }
}
void LightingService::fillAll(const QColor& c) {
    for (const auto& k : layout()) m_perKey[k.lightIndex] = Rgb{uint8_t(c.red()), uint8_t(c.green()), uint8_t(c.blue())};
    editedPerKey();
    emit perKeyChanged();
    if (m_perKeyActive) scheduleLive();
}
// Painting means the editor (not the board's last table) is what the user wants to see.
void LightingService::editedPerKey() { if (m_deviceShown) { m_deviceShown = false; emit syncChanged(); } }
void LightingService::clearPerKey() { m_perKey.clear(); editedPerKey(); emit perKeyChanged(); if (m_perKeyActive) scheduleLive(); }
void LightingService::readPerKeyFromDevice() { m_importRead = true; m_dev->readPerKeyColors(); }
QVariantMap LightingService::deviceColors() const {
    QVariantMap v;
    for (auto it = m_device.begin(); it != m_device.end(); ++it) v[QString::number(it.key())] = QColor(it->r, it->g, it->b);
    return v;
}
void LightingService::beginStroke() { m_stroke = true; m_live.stop(); }
void LightingService::endStroke() { if (!m_stroke) return; m_stroke = false; if (m_perKeyActive) scheduleLive(); }

// ---- colour correction ----------------------------------------------------
void LightingService::saveCorrection() const {
    SettingsRepository st;
    st.set("lighting/correction/enabled", m_cc.enabled);
    st.set("lighting/correction/gamma", m_cc.gamma);
    st.set("lighting/correction/gainR", m_cc.gainR);
    st.set("lighting/correction/gainG", m_cc.gainG);
    st.set("lighting/correction/gainB", m_cc.gainB);
}
void LightingService::correctionTouched() {
    saveCorrection();
    emit correctionChanged();
    if (m_calibrating) m_live.start();   // calibration always previews live
    else scheduleLive();
}
void LightingService::setCorrectionEnabled(bool on) { if (on != m_cc.enabled) { m_cc.enabled = on; correctionTouched(); } }
void LightingService::setGamma(double v) { v = qBound(1.0, v, 3.0); if (!qFuzzyCompare(v, m_cc.gamma)) { m_cc.gamma = v; correctionTouched(); } }
void LightingService::setGainR(double v) { v = qBound(0.2, v, 1.0); if (!qFuzzyCompare(v, m_cc.gainR)) { m_cc.gainR = v; correctionTouched(); } }
void LightingService::setGainG(double v) { v = qBound(0.2, v, 1.0); if (!qFuzzyCompare(v, m_cc.gainG)) { m_cc.gainG = v; correctionTouched(); } }
void LightingService::setGainB(double v) { v = qBound(0.2, v, 1.0); if (!qFuzzyCompare(v, m_cc.gainB)) { m_cc.gainB = v; correctionTouched(); } }
void LightingService::resetCorrection() { m_cc = ColorCorrection{}; correctionTouched(); }
void LightingService::setCalibrating(bool on) {
    if (on == m_calibrating) return;
    m_calibrating = on;
    emit correctionChanged();
    if (on) send(calibrationTx(), "Calibration: white", false);   // explicit user action
    else applyCurrent();                                          // restore the user's lighting
}

// ---- sending --------------------------------------------------------------
LightingState LightingService::corrected(LightingState s) const {
    const Rgb c = m_cc.apply(Rgb{s.r, s.g, s.b});
    s.r = c.r; s.g = c.g; s.b = c.b;
    return s;
}
Transaction LightingService::effectTx() const { return CommandBuilder::lighting(corrected(m_s)); }
Transaction LightingService::calibrationTx() const {
    LightingState w; w.mode = 1; w.brightness = m_s.brightness ? m_s.brightness : uint8_t(kBrightnessMax);
    w.r = w.g = w.b = 0xFF;                       // static white, through the correction
    return CommandBuilder::lighting(corrected(w));
}

bool LightingService::send(const Transaction& tx, const QString& label, bool live) {
    const QByteArray b = bytesOf(tx);
    if (live && b == m_lastSent) return true;   // keyboard already shows exactly this
    if (!m_dev->submit(tx, label, "lighting")) return false;
    m_lastSent = b;
    return true;
}

bool LightingService::apply() {
    if (m_perKeyActive) { m_perKeyActive = false; emit changed(); }
    m_boardInUserMode = false;
    const auto* m = findMode(m_s.mode);
    return send(effectTx(), QString("Lighting: %1").arg(m ? m->name : "?"), false);
}
bool LightingService::applyPerKey() {
    if (!m_perKeyActive) { m_perKeyActive = true; emit changed(); }
    const bool ok = send(CommandBuilder::perKeyColors(m_cc.apply(m_perKey), m_s.brightness, true),
                         QString("Per-key colours (%1 keys)").arg(m_perKey.size()), false);
    if (ok) { m_boardInUserMode = true; m_userModeBrightness = m_s.brightness; }
    return ok;
}
bool LightingService::applyCurrent() { return m_perKeyActive ? applyPerKey() : apply(); }

// Live updates: smallest write that gets the board to the new state, nothing if it is unchanged.
void LightingService::liveApply() {
    if (m_calibrating) { send(calibrationTx(), "Calibration: white", true); m_boardInUserMode = false; return; }
    if (!m_perKeyActive) {
        m_boardInUserMode = false;
        const auto* m = findMode(m_s.mode);
        send(effectTx(), QString("Lighting: %1").arg(m ? m->name : "?"), true);
        return;
    }
    // Already in user mode with the same brightness: upload only the 04 23 table. Re-sending the
    // 0x80 mode switch first is what makes the whole board blink on every update.
    const bool switchMode = !(m_boardInUserMode && m_userModeBrightness == m_s.brightness);
    if (send(CommandBuilder::perKeyColors(m_cc.apply(m_perKey), m_s.brightness, switchMode),
             QString("Per-key colours (%1 keys)%2").arg(m_perKey.size()).arg(switchMode ? "" : ", table only"), true)) {
        m_boardInUserMode = true; m_userModeBrightness = m_s.brightness;
    }
}

// ---- persistence ----------------------------------------------------------
QJsonObject LightingService::toJson() const {
    QJsonObject pk;
    for (auto it = m_perKey.begin(); it != m_perKey.end(); ++it)
        pk[QString::number(it.key())] = QColor(it->r, it->g, it->b).name();
    return {{"mode", m_s.mode}, {"brightness", m_s.brightness}, {"speed", m_s.speed},
            {"direction", int(m_s.direction)}, {"colorful", m_s.colorful},
            {"color", color().name()}, {"perKey", pk}, {"perKeyActive", m_perKeyActive}};
}
void LightingService::fromJson(const QJsonObject& o) {
    if (o.isEmpty()) return;
    LightingState s;
    const int mode = o["mode"].toInt(kDefaultMode);
    s.mode = findMode(uint8_t(mode)) ? uint8_t(mode) : kDefaultMode;
    s.brightness = uint8_t(qBound(0, o["brightness"].toInt(kDefaultBrightness), kBrightnessMax));
    s.speed = uint8_t(qBound(0, o["speed"].toInt(kDefaultSpeed), kSpeedMax));
    s.direction = Direction(qBound(0, o["direction"].toInt(), 3));
    s.colorful = o["colorful"].toBool();
    const QColor c(o["color"].toString("#ffffff"));
    s.r = uint8_t(c.red()); s.g = uint8_t(c.green()); s.b = uint8_t(c.blue());
    m_s = s;
    if (m_deviceShown) { m_deviceShown = false; emit syncChanged(); }
    m_perKeyActive = o["perKeyActive"].toBool();
    m_perKey.clear();
    const auto pk = o["perKey"].toObject();
    for (auto it = pk.begin(); it != pk.end(); ++it) {
        const int i = it.key().toInt(); const QColor k(it.value().toString());
        if (i > 0 && i < kSlotCount - 1 && k.isValid()) m_perKey[i] = Rgb{uint8_t(k.red()), uint8_t(k.green()), uint8_t(k.blue())};
    }
    emit changed(); emit perKeyChanged();
}
}
