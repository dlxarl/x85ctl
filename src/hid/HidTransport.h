#pragma once
#include <QByteArray>
#include <QString>

namespace kcc {
// Minimal byte transport for 64-byte feature reports with report ID 0.
// Abstract so the protocol engine can be tested without hardware.
class HidTransport {
public:
    virtual ~HidTransport() = default;
    virtual bool isOpen() const = 0;
    virtual bool sendFeature(const QByteArray& payload64) = 0;  // report ID added internally
    virtual QByteArray getFeature(int len = 64) = 0;            // returns payload without report ID
    virtual QString lastError() const = 0;
};
}
