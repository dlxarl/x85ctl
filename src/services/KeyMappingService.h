#pragma once
#include "protocol/CommandBuilder.h"
#include <QJsonObject>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace kcc {
class DeviceManager;
class MacroService;

class KeyMappingService : public QObject {
    Q_OBJECT
    Q_PROPERTY(int layer READ layer WRITE setLayer NOTIFY changed)          // 0 base, 1 Fn
    Q_PROPERTY(QVariantMap labels READ labels NOTIFY changed)                // keyIndex -> label (current layer)
    Q_PROPERTY(QVariantList keyTargets READ keyTargets CONSTANT)
    Q_PROPERTY(QVariantList mediaTargets READ mediaTargets CONSTANT)
    Q_PROPERTY(QVariantList mouseTargets READ mouseTargets CONSTANT)
public:
    KeyMappingService(DeviceManager* dev, QObject* parent = nullptr);
    int layer() const { return m_layer; }
    void setLayer(int l) { l = l ? 1 : 0; if (l != m_layer) { m_layer = l; emit changed(); } }
    QVariantMap labels() const;
    QVariantList keyTargets() const;
    QVariantList mediaTargets() const;
    QVariantList mouseTargets() const;

    Q_INVOKABLE QString describe(int keyIndex) const;
    bool usesMacros() const;
    Q_INVOKABLE void assignKey(int keyIndex, int usage, int modifiers);
    Q_INVOKABLE void assignMedia(int keyIndex, int usage);
    Q_INVOKABLE void assignMouse(int keyIndex, const QString& kind, int value);
    Q_INVOKABLE void assignMacro(int keyIndex, int macroIndex, int mode, int count);
    Q_INVOKABLE void reset(int keyIndex);
    Q_INVOKABLE void resetLayer();
    Q_INVOKABLE bool apply();

    void setMacros(MacroService* m);   // names for labels; uploaded before a layer that uses macros
    QString describe(const KeyAction& a) const;
    static QString describeStatic(const KeyAction& a);
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& o);
    const QMap<int, KeyAction>& layerMap(int l) const { return m_map[l ? 1 : 0]; }
signals:
    void changed();
private:
    void set(int keyIndex, const KeyAction& a);
    DeviceManager* m_dev;
    MacroService* m_macros = nullptr;
    int m_layer = 0;
    QMap<int, KeyAction> m_map[2];
};
}
