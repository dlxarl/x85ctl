#include "DeviceDiscovery.h"
#include "Platform.h"
#include "protocol/X85Protocol.h"
#include <hidapi.h>
#include <QMap>
#include <algorithm>
#include <tuple>

namespace kcc {
const QVector<SupportedDevice>& DeviceDiscovery::supported() {
    // 3151:502D (Sharkfin/ROYUAN family) is deliberately absent: different protocol.
    static const QVector<SupportedDevice> s = {
        {"ATTACK SHARK X85", x85::kVid, x85::kPid, x85::kUsagePage, x85::kUsage},
    };
    return s;
}

static QString w(const wchar_t* s) { return s ? QString::fromWCharArray(s) : QString(); }

QVector<DeviceInfo> DeviceDiscovery::scan() {
    static const bool init = [] { hid_init(); platform::prepareHid(); return true; }();
    Q_UNUSED(init);
    QMap<QString, DeviceInfo> byDevice;   // key: VID:PID:serial
    hid_device_info* head = hid_enumerate(0, 0);
    for (auto* d = head; d; d = d->next) {
        const SupportedDevice* s = nullptr;
        for (const auto& c : supported()) if (c.vid == d->vendor_id && c.pid == d->product_id) s = &c;
        if (!s) continue;
        HidInterfaceInfo i{QString::fromUtf8(d->path), d->usage_page, d->usage, d->interface_number};
        if (i.usagePage == s->configUsagePage && i.usage == s->configUsage) i.role = InterfaceRole::Config;
        else if (i.usagePage == 0x01 && i.usage == 0x06) i.role = InterfaceRole::Keyboard;
        else if (i.usagePage == 0x0C) i.role = InterfaceRole::Consumer;
        const QString key = QString("%1:%2:%3").arg(d->vendor_id).arg(d->product_id).arg(w(d->serial_number));
        DeviceInfo& di = byDevice[key];
        di.model = s->model; di.vendorId = d->vendor_id; di.productId = d->product_id;
        di.releaseBcd = d->release_number;
        di.manufacturer = w(d->manufacturer_string); di.product = w(d->product_string); di.serial = w(d->serial_number);
        di.interfaces << i;
    }
    hid_free_enumeration(head);
    QVector<DeviceInfo> out = byDevice.values().toVector();
    for (auto& d : out)
        std::sort(d.interfaces.begin(), d.interfaces.end(), [](const auto& a, const auto& b) {
            return std::tie(a.interfaceNumber, a.usagePage, a.usage) < std::tie(b.interfaceNumber, b.usagePage, b.usage);
        });
    return out;
}
}
