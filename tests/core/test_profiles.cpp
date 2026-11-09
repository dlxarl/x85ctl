#include "storage/ProfileRepository.h"
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>
using namespace kcc;

class ProfilesTest : public QObject {
    Q_OBJECT
private slots:
    void roundTrip() {
        QTemporaryDir t; ProfileRepository r(t.path());
        QVERIFY(r.save("Work/Night", {{"device", "x85"}, {"k", 1}}));
        QCOMPARE(r.names(), QStringList{"Work_Night"});
        QCOMPARE(r.load("Work_Night")["k"].toInt(), 1);
        QCOMPARE(r.load("Work_Night")["schema"].toInt(), 1);
    }
    void exportImport() {
        QTemporaryDir t; ProfileRepository r(t.path() + "/p");
        r.save("A", {{"device", "x85"}});
        QVERIFY(r.exportTo("A", t.path() + "/A.json"));
        QCOMPARE(r.importFrom(t.path() + "/A.json"), QString("A 2"));   // no overwrite
    }
    void rejectsForeignOrBroken() {
        QTemporaryDir t; ProfileRepository r(t.path() + "/p"); QString err;
        auto write = [&](const char* n, const QByteArray& b) { QFile f(t.path() + "/" + n); f.open(QIODevice::WriteOnly); f.write(b); };
        write("bad.json", "nope");
        write("other.json", R"({"schema":1,"device":"k86"})");
        write("future.json", R"({"schema":9,"device":"x85"})");
        QVERIFY(r.importFrom(t.path() + "/bad.json", &err).isEmpty());
        QVERIFY(r.importFrom(t.path() + "/other.json", &err).isEmpty());
        QVERIFY(r.importFrom(t.path() + "/future.json", &err).isEmpty());
    }
};
QTEST_APPLESS_MAIN(ProfilesTest)
#include "test_profiles.moc"
