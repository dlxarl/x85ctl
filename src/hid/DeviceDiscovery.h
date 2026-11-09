#pragma once
#include "core/DeviceInfo.h"
#include <QVector>

namespace kcc {
// Supported hardware: matched on VID/PID AND the vendor collection usage, never by name.
struct SupportedDevice {
    const char* model;
    uint16_t vid, pid, configUsagePage, configUsage;
};

class DeviceDiscovery {
public:
    static const QVector<SupportedDevice>& supported();
    static QVector<DeviceInfo> scan();
};
}
