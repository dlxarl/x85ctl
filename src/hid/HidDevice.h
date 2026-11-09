#pragma once
#include "HidReport.h"
#include "HidTransport.h"
#include <optional>

struct hid_device_;

namespace kcc {
class HidDevice final : public HidTransport {
public:
    HidDevice() = default;
    ~HidDevice() override;
    HidDevice(const HidDevice&) = delete;
    HidDevice& operator=(const HidDevice&) = delete;

    bool open(const QString& path);
    void close();
    bool isOpen() const override { return m_dev != nullptr; }
    bool sendFeature(const QByteArray& payload) override;
    QByteArray getFeature(int len = 64) override;
    QString lastError() const override { return m_err; }

    QByteArray reportDescriptor() const;
    std::optional<QByteArray> readInput(int timeoutMs);
private:
    hid_device_* m_dev = nullptr;
    QString m_err;
};
}
