#include "DeviceConnection.h"
#include "protocol/ProtocolParser.h"
#include <QColor>
#include <QThread>

namespace kcc {
TransactionRunner DeviceConnection::runner() {
    return TransactionRunner(*m_dev, [](int ms) { QThread::msleep(ms); },
                             [this](const QString& s) { emit log(s); });
}

void DeviceConnection::open(const QString& path) {
    close();
    auto d = std::make_unique<HidDevice>();
    if (!d->open(path)) { emit opened(false, d->lastError()); return; }
    m_dev = std::move(d);
    emit opened(true, {});
}
void DeviceConnection::close() { m_dev.reset(); }

void DeviceConnection::run(const Transaction& tx, const QString& label) {
    if (!m_dev) { emit finished(label, false, "Not connected"); return; }
    emit log(QStringLiteral("— %1 (%2 packets)").arg(label).arg(tx.size()));
    auto r = runner().run(tx);
    emit finished(label, r.ok, r.error);
}

void DeviceConnection::readPerKeyColors() {
    if (!m_dev) { emit perKeyColorsRead({}, "Not connected"); return; }
    emit log("— Read per-key colours (04 F5)");
    QString err;
    const QByteArray t = runner().readPerKeyColors(&err);
    QVariantMap out;
    for (auto [i, c] : ProtocolParser::parseColorTable(t).asKeyValueRange())
        out.insert(QString::number(i), QColor(c.r, c.g, c.b));
    emit perKeyColorsRead(out, err);
}

void DeviceConnection::readDescriptor(const QString& path) {
    HidDevice d;
    if (!d.open(path)) { emit descriptorRead("Cannot open: " + d.lastError()); return; }
    const QByteArray raw = d.reportDescriptor();
    auto s = ProtocolParser::parseDescriptor(raw);
    QString text = "Raw: " + QString::fromLatin1(raw.toHex(' ')) + "\n\n" + s.text;
    for (const auto& r : s.reports) text += QString("\n%1 id=%2: %3 bytes").arg(r.kind).arg(r.reportId).arg(r.bits / 8);
    emit descriptorRead(text);
}
}
