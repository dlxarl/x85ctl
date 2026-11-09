#pragma once
#include "TransactionRunner.h"
#include "hid/HidDevice.h"
#include <QObject>
#include <QVariantMap>
#include <memory>

namespace kcc {
// Lives on a worker thread; owns the open handle to the config interface.
class DeviceConnection : public QObject {
    Q_OBJECT
public:
    void open(const QString& path);
    void close();
    void run(const Transaction& tx, const QString& label);
    void readPerKeyColors();
    void readDescriptor(const QString& path);
signals:
    void opened(bool ok, const QString& error);
    void log(const QString& line);
    void finished(const QString& label, bool ok, const QString& error);
    void perKeyColorsRead(const QVariantMap& byLightIndex, const QString& error);
    void descriptorRead(const QString& text);
private:
    std::unique_ptr<HidDevice> m_dev;
    TransactionRunner runner();
};
}
