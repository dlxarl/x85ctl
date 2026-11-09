#include "ProfileRepository.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace kcc {
static constexpr int kSchema = 1;

ProfileRepository::ProfileRepository(QString dir) : m_dir(std::move(dir)) { QDir().mkpath(m_dir); }
QString ProfileRepository::defaultDir() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles";
}
QString ProfileRepository::sanitize(const QString& n) {
    QString s = n.trimmed();
    s.replace(QRegularExpression("[^\\w .-]", QRegularExpression::UseUnicodePropertiesOption), "_");
    while (s.startsWith('.')) s.remove(0, 1);
    return s.left(64).trimmed();
}
QString ProfileRepository::path(const QString& n) const { return m_dir + "/" + sanitize(n) + ".json"; }
QStringList ProfileRepository::names() const {
    QStringList l;
    for (const auto& f : QDir(m_dir).entryInfoList({"*.json"}, QDir::Files, QDir::Name)) l << f.completeBaseName();
    return l;
}
bool ProfileRepository::exists(const QString& n) const { return QFile::exists(path(n)); }
bool ProfileRepository::save(const QString& n, const QJsonObject& data) const {
    if (sanitize(n).isEmpty()) return false;
    // Snapshot/screenshot runs load the user's profiles with demo data: never write them back.
    if (qEnvironmentVariableIsSet("KCC_SNAPSHOT_DIR")) return true;
    QJsonObject o = data; o["schema"] = kSchema; o["name"] = sanitize(n);
    QSaveFile f(path(n));                       // atomic write
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return f.commit();
}
QJsonObject ProfileRepository::load(const QString& n) const {
    QFile f(path(n));
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}
bool ProfileRepository::remove(const QString& n) const { return QFile::remove(path(n)); }
bool ProfileRepository::exportTo(const QString& n, const QString& dst) const {
    const auto o = load(n);
    if (o.isEmpty()) return false;
    QSaveFile f(dst);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return f.commit();
}
QString ProfileRepository::importFrom(const QString& src, QString* err) const {
    auto fail = [&](const QString& e) { if (err) *err = e; return QString(); };
    QFile f(src);
    if (!f.open(QIODevice::ReadOnly)) return fail("Cannot open file");
    if (f.size() > 1 << 20) return fail("File too large");
    QJsonParseError pe;
    const auto doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) return fail("Not a JSON object");
    const auto o = doc.object();
    if (o["schema"].toInt() != kSchema) return fail("Unsupported profile schema");
    if (o["device"].toString() != "x85") return fail("Profile is for a different device");
    QString name = sanitize(o["name"].toString(QFileInfo(src).completeBaseName()));
    if (name.isEmpty()) name = "Imported";
    const QString base = name;
    for (int i = 2; exists(name) && i < 1000; ++i) name = base + " " + QString::number(i);
    return save(name, o) ? name : fail("Cannot write profile");
}
}
