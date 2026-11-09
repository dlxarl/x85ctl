#include "core/TransactionRunner.h"
#include <QtTest>
#include <deque>
using namespace kcc;

struct MockTransport : HidTransport {
    QVector<QByteArray> sent;
    std::deque<QByteArray> replies;
    int failSends = 0;
    bool isOpen() const override { return true; }
    bool sendFeature(const QByteArray& p) override { if (failSends > 0) { --failSends; return false; } sent << p; return true; }
    QByteArray getFeature(int len) override {
        if (replies.empty()) return QByteArray(len, 0);
        auto r = replies.front(); replies.pop_front(); return r;
    }
    QString lastError() const override { return "mock"; }
};

class RunnerTest : public QObject {
    Q_OBJECT
    int slept = 0;
    TransactionRunner make(MockTransport& m) { return TransactionRunner(m, [this](int ms) { slept += ms; }, [](const QString&) {}); }
private slots:
    void sendsAllPacketsWithPacing() {
        MockTransport m; slept = 0;
        const auto tx = CommandBuilder::lighting({});
        QVERIFY(make(m).run(tx).ok);
        QCOMPARE(m.sent.size(), tx.size());
        QVERIFY(slept >= 35 * tx.size());
    }
    void retriesFailedSend() {
        MockTransport m; m.failSends = 3;
        QVERIFY(make(m).run(CommandBuilder::lighting({})).ok);
        QCOMPARE(m.sent.size(), 5);
    }
    void givesUpAfterTen() {
        MockTransport m; m.failSends = 100;
        QVERIFY(!make(m).run(CommandBuilder::lighting({})).ok);
    }
    void resendsWhenBusy() {
        MockTransport m;
        QByteArray busy(64, 0); busy[3] = char(0xFF);
        m.replies = {busy};                       // reply to Begin is busy once
        QVERIFY(make(m).run(CommandBuilder::lighting({})).ok);
        QCOMPARE(m.sent.size(), 6);
        QCOMPARE(m.sent[0], m.sent[1]);
    }
    void readsColorTable() {
        MockTransport m;
        QByteArray ack(64, 0); ack[0] = 4; ack[1] = char(0xF5); ack[3] = 1;
        m.replies.push_back(ack);
        for (int p = 0; p < 9; ++p) { QByteArray d(64, 0); if (p == 0) { d[4] = 1; d[5] = char(0xAB); } m.replies.push_back(d); }
        QString err;
        const auto t = make(m).readPerKeyColors(&err);
        QCOMPARE(t.size(), 576);
        QCOMPARE(uint8_t(t[5]), uint8_t(0xAB));
        QCOMPARE(uint8_t(m.sent.first()[1]), uint8_t(0xF5));
        QCOMPARE(uint8_t(m.sent.last()[1]), uint8_t(0x02));
    }
    void readFailsWithoutAck() {
        MockTransport m; QString err;
        QVERIFY(make(m).readPerKeyColors(&err).isEmpty());
        QVERIFY(!err.isEmpty());
        QCOMPARE(uint8_t(m.sent.last()[1]), uint8_t(0x02));   // still closes the session
    }
};
QTEST_APPLESS_MAIN(RunnerTest)
#include "test_runner.moc"
