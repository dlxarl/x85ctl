#pragma once
#include <QByteArray>
#include <QString>

namespace kcc {
struct HidReport {
    enum class Kind { Input, Output, FeatureOut, FeatureIn };
    Kind kind = Kind::Input;
    QByteArray data;
    QString hex() const { return QString::fromLatin1(data.toHex(' ')); }
    static QString tag(Kind k) {
        switch (k) { case Kind::Input: return "IN  "; case Kind::Output: return "OUT ";
                     case Kind::FeatureOut: return "SETF"; default: return "GETF"; }
    }
};
}
