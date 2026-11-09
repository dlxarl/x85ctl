#pragma once
#include "CommandBuilder.h"
#include <QByteArray>
#include <QMap>
#include <QString>
#include <QVector>

namespace kcc {
struct ReportSummary { int reportId = 0; QString kind; int bits = 0; };
struct DescriptorSummary { QString text; QVector<ReportSummary> reports; bool valid = true; };

class ProtocolParser {
public:
    static DescriptorSummary parseDescriptor(const QByteArray& d);
    // Reply status byte (offset 3): 0x01 = data follows, 0xFF = busy.
    static bool isBusy(const QByteArray& reply) { return reply.size() > 3 && uint8_t(reply[3]) == 0xFF; }
    static bool hasData(const QByteArray& reply) { return reply.size() > 3 && uint8_t(reply[3]) == 0x01; }
    // Decodes 576 bytes of [idx,R,G,B] slots.
    static QMap<int, Rgb> parseColorTable(const QByteArray& table);
};
}
