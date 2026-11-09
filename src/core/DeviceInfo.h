#pragma once
#include <QString>
#include <QVector>
#include <cstdint>

namespace kcc {
enum class InterfaceRole { Unknown, Keyboard, Consumer, Config };

struct HidInterfaceInfo {
    QString path;
    uint16_t usagePage = 0, usage = 0;
    int interfaceNumber = -1;
    InterfaceRole role = InterfaceRole::Unknown;
};

struct DeviceInfo {
    QString model;
    QString manufacturer, product, serial;
    uint16_t vendorId = 0, productId = 0, releaseBcd = 0;
    QVector<HidInterfaceInfo> interfaces;
    const HidInterfaceInfo* configInterface() const {
        for (const auto& i : interfaces) if (i.role == InterfaceRole::Config) return &i;
        return nullptr;
    }
};
}
