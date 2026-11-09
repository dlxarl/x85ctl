#include "protocol/ProtocolParser.h"
#include <QtTest>
using namespace kcc;

class ParserTest : public QObject {
    Q_OBJECT
private slots:
    void vendorDescriptor() {   // captured from the physical X85
        const auto s = ProtocolParser::parseDescriptor(
            QByteArray::fromHex("0613ff0901a101150026ff007508954009028102090391020a00ffb102c0"));
        QVERIFY(s.valid);
        QCOMPARE(s.reports.size(), 3);
        for (const auto& r : s.reports) { QCOMPARE(r.reportId, 0); QCOMPARE(r.bits, 512); }
    }
    void truncated() { QVERIFY(!ProtocolParser::parseDescriptor(QByteArray::fromHex("0613")).valid); }
    void status() {
        QByteArray r(64, 0); r[3] = char(0xFF);
        QVERIFY(ProtocolParser::isBusy(r));
        r[3] = 1; QVERIFY(ProtocolParser::hasData(r));
    }
    void colorTable() {
        QByteArray t(576, 0);
        t[8] = 2; t[9] = char(0x10); t[10] = char(0x20); t[11] = char(0x30);
        t[12] = 9;   // wrong index tag -> ignored
        const auto m = ProtocolParser::parseColorTable(t);
        QCOMPARE(m.size(), 1);
        QCOMPARE(m[2].g, uint8_t(0x20));
    }
};
QTEST_APPLESS_MAIN(ParserTest)
#include "test_parser.moc"
