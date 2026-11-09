#pragma once
#include <QJsonObject>
#include <QStringList>

namespace kcc {
// One JSON file per profile in <AppData>/profiles. Names are sanitised to safe file names.
class ProfileRepository {
public:
    explicit ProfileRepository(QString dir = defaultDir());
    static QString defaultDir();
    static QString sanitize(const QString& name);
    QStringList names() const;
    bool exists(const QString& name) const;
    bool save(const QString& name, const QJsonObject& data) const;
    QJsonObject load(const QString& name) const;
    bool remove(const QString& name) const;
    bool exportTo(const QString& name, const QString& path) const;
    QString importFrom(const QString& path, QString* error = nullptr) const;
    QString dir() const { return m_dir; }
private:
    QString path(const QString& name) const;
    QString m_dir;
};
}
