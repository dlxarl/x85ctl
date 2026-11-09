#pragma once
#include "ColorCorrection.h"
#include "protocol/CommandBuilder.h"
#include <QColor>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

namespace kcc {
class DeviceManager;

class LightingService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList modes READ modes CONSTANT)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY changed)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY changed)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY changed)
    Q_PROPERTY(int direction READ direction WRITE setDirection NOTIFY changed)
    Q_PROPERTY(bool colorful READ colorful WRITE setColorful NOTIFY changed)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY changed)
    Q_PROPERTY(int caps READ caps NOTIFY changed)
    Q_PROPERTY(int brightnessMax READ brightnessMax CONSTANT)
    Q_PROPERTY(int speedMax READ speedMax CONSTANT)
    Q_PROPERTY(QVariantMap perKey READ perKey NOTIFY perKeyChanged)
    Q_PROPERTY(bool autoApply READ autoApply WRITE setAutoApply NOTIFY changed)
    // true: the board shows the per-key colour table (user mode 0x80); false: an effect from `mode`.
    Q_PROPERTY(bool perKeyActive READ perKeyActive WRITE setPerKeyActive NOTIFY changed)
    // Colour correction (gamma + white balance), applied to every colour sent to the keyboard.
    Q_PROPERTY(bool correctionEnabled READ correctionEnabled WRITE setCorrectionEnabled NOTIFY correctionChanged)
    Q_PROPERTY(double gamma READ gamma WRITE setGamma NOTIFY correctionChanged)
    Q_PROPERTY(double gainR READ gainR WRITE setGainR NOTIFY correctionChanged)
    Q_PROPERTY(double gainG READ gainG WRITE setGainG NOTIFY correctionChanged)
    Q_PROPERTY(double gainB READ gainB WRITE setGainB NOTIFY correctionChanged)
    // While true the keyboard shows corrected white (static) so the white balance can be tuned by eye.
    Q_PROPERTY(bool calibrating READ calibrating WRITE setCalibrating NOTIFY correctionChanged)
    // What the editor shows relative to the keyboard (read on connect / last saved).
    Q_PROPERTY(QString syncInfo READ syncInfo NOTIFY syncChanged)
    // true while the stage should show the colours read from the keyboard (until the user edits something).
    Q_PROPERTY(bool deviceShown READ deviceShown NOTIFY syncChanged)
    Q_PROPERTY(QVariantMap deviceColors READ deviceColors NOTIFY syncChanged)
public:
    LightingService(DeviceManager* dev, QObject* parent = nullptr);

    QVariantList modes() const;
    int mode() const { return m_s.mode; }
    int brightness() const { return m_s.brightness; }
    int speed() const { return m_s.speed; }
    int direction() const { return int(m_s.direction); }
    bool colorful() const { return m_s.colorful; }
    QColor color() const { return QColor(m_s.r, m_s.g, m_s.b); }
    int caps() const;
    int brightnessMax() const { return x85::kBrightnessMax; }
    int speedMax() const { return x85::kSpeedMax; }
    QVariantMap perKey() const;
    bool autoApply() const { return m_auto; }
    bool perKeyActive() const { return m_perKeyActive; }
    bool correctionEnabled() const { return m_cc.enabled; }
    double gamma() const { return m_cc.gamma; }
    double gainR() const { return m_cc.gainR; }
    double gainG() const { return m_cc.gainG; }
    double gainB() const { return m_cc.gainB; }
    bool calibrating() const { return m_calibrating; }
    QString syncInfo() const { return m_sync; }
    bool deviceShown() const { return m_deviceShown; }
    QVariantMap deviceColors() const;

    void setMode(int);
    void setBrightness(int);
    void setSpeed(int);
    void setDirection(int);
    void setColorful(bool);
    void setColor(const QColor&);
    void setAutoApply(bool on);
    void setPerKeyActive(bool on);
    void setCorrectionEnabled(bool on);
    void setGamma(double v);
    void setGainR(double v);
    void setGainG(double v);
    void setGainB(double v);
    void setCalibrating(bool on);

    // Explicit uploads (buttons, profiles): always the full vendor sequence.
    Q_INVOKABLE bool apply();
    Q_INVOKABLE bool applyPerKey();
    Q_INVOKABLE bool applyCurrent();          // effect or per-key, whichever is active
    Q_INVOKABLE void setKeyColor(int lightIndex, const QColor& c);
    Q_INVOKABLE void clearKey(int lightIndex);
    Q_INVOKABLE void fillAll(const QColor& c);
    Q_INVOKABLE void clearPerKey();
    Q_INVOKABLE void readPerKeyFromDevice();
    Q_INVOKABLE void resetCorrection();
    Q_INVOKABLE void beginStroke();           // painting: hold live uploads until the stroke ends
    Q_INVOKABLE void endStroke();

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& o);
    const LightingState& state() const { return m_s; }
    const QMap<int, Rgb>& perKeyMap() const { return m_perKey; }
    const ColorCorrection& correction() const { return m_cc; }

signals:
    void changed();
    void perKeyChanged();
    void correctionChanged();
    void syncChanged();
private:
    void touched();
    void editedPerKey();
    void scheduleLive();
    void liveApply();
    void correctionTouched();
    void saveCorrection() const;
    LightingState corrected(LightingState s) const;
    Transaction effectTx() const;
    Transaction calibrationTx() const;
    bool send(const Transaction& tx, const QString& label, bool live);

    DeviceManager* m_dev;
    LightingState m_s;
    QMap<int, Rgb> m_perKey;
    ColorCorrection m_cc;
    bool m_auto = false;
    bool m_perKeyActive = false;
    bool m_calibrating = false;
    bool m_stroke = false;
    bool m_boardInUserMode = false;   // we put the board into mode 0x80 and nothing changed it since
    uint8_t m_userModeBrightness = 0;
    QByteArray m_lastSent;            // bytes of the last lighting transaction (live de-duplication)
    QTimer m_live;
    bool m_wasReady = false;
    QString m_sync;
    bool m_deviceShown = false;
    bool m_importRead = false;        // explicit Read button: copy the board's table into the editor/profile
    QMap<int, Rgb> m_device;          // last table read from the board (never saved into a profile)
};
}
