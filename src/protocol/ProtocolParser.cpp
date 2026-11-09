#include "ProtocolParser.h"

namespace kcc {

static QString pageName(uint32_t p) {
    switch (p) { case 1: return "Generic Desktop"; case 7: return "Keyboard"; case 8: return "LED";
                 case 9: return "Button"; case 0xC: return "Consumer"; }
    return p >= 0xFF00 ? "Vendor-defined" : QString();
}

DescriptorSummary ProtocolParser::parseDescriptor(const QByteArray& d) {
    DescriptorSummary out;
    QMap<QPair<int, QString>, int> agg;
    uint32_t size = 0, count = 0, rid = 0;
    int depth = 0;
    for (int i = 0; i < d.size();) {
        const uint8_t b = uint8_t(d[i]);
        if (b == 0xFE) { out.valid = false; break; }
        const int sz = (b & 3) == 3 ? 4 : (b & 3), type = (b >> 2) & 3, tag = b >> 4;
        if (i + 1 + sz > d.size()) { out.valid = false; break; }
        uint32_t v = 0;
        for (int k = 0; k < sz; ++k) v |= uint32_t(uint8_t(d[i + 1 + k])) << (8 * k);
        QString name, val = sz ? QString("0x%1").arg(v, 0, 16) : QString();
        if (type == 1) {
            switch (tag) {
            case 0: name = "Usage Page"; if (auto n = pageName(v); !n.isEmpty()) val += " (" + n + ")"; break;
            case 1: name = "Logical Min"; val = QString::number(int(v)); break;
            case 2: name = "Logical Max"; val = QString::number(v); break;
            case 7: name = "Report Size"; size = v; val = QString::number(v); break;
            case 8: name = "Report ID"; rid = v; val = QString::number(v); break;
            case 9: name = "Report Count"; count = v; val = QString::number(v); break;
            default: name = "Global"; }
        } else if (type == 2) {
            name = tag == 0 ? "Usage" : tag == 1 ? "Usage Min" : tag == 2 ? "Usage Max" : "Local";
        } else if (type == 0) {
            QString k;
            switch (tag) {
            case 8: name = k = "Input"; break;
            case 9: name = k = "Output"; break;
            case 11: name = k = "Feature"; break;
            case 10: name = "Collection"; break;
            case 12: name = "End Collection"; --depth; break;
            default: name = "Main"; }
            if (!k.isEmpty()) agg[{int(rid), k}] += int(size * count);
        }
        out.text += QString(qMax(0, depth) * 2, ' ') + name + (val.isEmpty() ? "" : " " + val) + "\n";
        if (type == 0 && tag == 10) ++depth;
        i += 1 + sz;
    }
    for (auto it = agg.begin(); it != agg.end(); ++it)
        out.reports.push_back({it.key().first, it.key().second, it.value()});
    return out;
}

QMap<int, Rgb> ProtocolParser::parseColorTable(const QByteArray& t) {
    QMap<int, Rgb> m;
    for (int i = 1; i + 3 < t.size() && i < x85::kSlotCount - 1; ++i) {
        const int o = i * 4;
        if (uint8_t(t[o]) != i) continue;
        m[i] = Rgb{uint8_t(t[o + 1]), uint8_t(t[o + 2]), uint8_t(t[o + 3])};
    }
    return m;
}
}
