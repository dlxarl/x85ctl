#pragma once
#include <QSettings>
#include <QVariant>
namespace kcc {
class SettingsRepository {
public:
    QVariant value(const QString& k, const QVariant& d = {}) const { return QSettings().value(k, d); }
    void set(const QString& k, const QVariant& v) {
        if (qEnvironmentVariableIsSet("KCC_SNAPSHOT_DIR")) return;   // snapshot runs never change user settings
        QSettings().setValue(k, v);
    }
};
}
