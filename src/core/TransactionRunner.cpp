#include "TransactionRunner.h"
#include "protocol/ProtocolParser.h"

namespace kcc {
static QString hex(const QByteArray& b, int n = 16) {
    QString s = QString::fromLatin1(b.left(n).toHex(' '));
    return b.size() > n ? s + " …" : s;
}

bool TransactionRunner::send(const QByteArray& p) {
    const bool ok = m_t.sendFeature(p);
    m_log(QStringLiteral("SETF %1%2").arg(hex(p), ok ? "" : "  ✗ " + m_t.lastError()));
    return ok;
}
QByteArray TransactionRunner::get() {
    QByteArray r = m_t.getFeature(x85::kPacketSize);
    m_log(QStringLiteral("GETF %1").arg(r.isEmpty() ? "✗ " + m_t.lastError() : hex(r)));
    return r;
}

TransactionRunner::Result TransactionRunner::run(const Transaction& tx) {
    if (!m_t.isOpen()) return {false, "Device not open"};
    for (const Packet& p : tx) {
        if (p.data.size() != x85::kPacketSize) return {false, "Malformed packet"};
        m_sleep(m_delay);
        int tries = 0;
        while (!send(p.data)) {
            if (++tries >= 10) return {false, "SetFeature failed: " + m_t.lastError()};
            m_sleep(m_delay);
        }
        if (!p.readReply) continue;
        m_sleep(m_delay);
        QByteArray r = get();
        for (int busy = 0; ProtocolParser::isBusy(r) && busy < 3; ++busy) {   // 0xFF → resend
            send(p.data); m_sleep(m_delay); r = get();
        }
    }
    return {};
}

QByteArray TransactionRunner::readPerKeyColors(QString* error) {
    auto fail = [&](const QString& e) { if (error) *error = e; return QByteArray(); };
    if (!m_t.isOpen()) return fail("Device not open");
    const auto req = CommandBuilder::readPerKeyColorsRequest().first().data;
    m_sleep(3);
    if (!send(req)) return fail("SetFeature failed: " + m_t.lastError());
    m_sleep(5);
    QByteArray table;
    QByteArray r = get();
    if (ProtocolParser::hasData(r)) {
        for (int page = 0; page < 9; ++page) {
            m_sleep(3);
            QByteArray d = get();
            if (d.size() != x85::kPacketSize) break;
            table += d;
        }
    }
    m_sleep(m_delay);
    send(CommandBuilder::header(x85::End));   // vendor app closes with 04 02
    m_sleep(m_delay);
    get();
    if (table.size() < x85::kTableBytes) return fail("Device returned no colour table (status byte 3 ≠ 0x01)");
    return table.left(x85::kTableBytes);
}
}
