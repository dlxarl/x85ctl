#include "HidDevice.h"
#include <hidapi.h>

namespace kcc {
HidDevice::~HidDevice() { close(); }

bool HidDevice::open(const QString& path) {
    close();
    m_dev = hid_open_path(path.toUtf8().constData());
    if (!m_dev) { m_err = QString::fromWCharArray(hid_error(nullptr)); return false; }
    return true;
}
void HidDevice::close() { if (m_dev) { hid_close(m_dev); m_dev = nullptr; } }

bool HidDevice::sendFeature(const QByteArray& p) {
    if (!m_dev) return false;
    QByteArray b(1, '\0'); b += p;   // report ID 0 → 65 bytes, like HidD_SetFeature(…, 0x41)
    const int n = hid_send_feature_report(m_dev, reinterpret_cast<const unsigned char*>(b.constData()), b.size());
    if (n < 0) m_err = QString::fromWCharArray(hid_error(m_dev));
    return n >= 0;
}
QByteArray HidDevice::getFeature(int len) {
    if (!m_dev) return {};
    QByteArray b(len + 1, '\0');
    const int n = hid_get_feature_report(m_dev, reinterpret_cast<unsigned char*>(b.data()), b.size());
    if (n < 0) { m_err = QString::fromWCharArray(hid_error(m_dev)); return {}; }
    return b.mid(1, n - 1);
}
QByteArray HidDevice::reportDescriptor() const {
    if (!m_dev) return {};
    unsigned char buf[HID_API_MAX_REPORT_DESCRIPTOR_SIZE];
    const int n = hid_get_report_descriptor(m_dev, buf, sizeof buf);
    return n > 0 ? QByteArray(reinterpret_cast<char*>(buf), n) : QByteArray();
}
std::optional<QByteArray> HidDevice::readInput(int timeoutMs) {
    if (!m_dev) return std::nullopt;
    unsigned char buf[256];
    const int n = hid_read_timeout(m_dev, buf, sizeof buf, timeoutMs);
    if (n <= 0) return std::nullopt;
    return QByteArray(reinterpret_cast<char*>(buf), n);
}
}
