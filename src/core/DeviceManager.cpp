#include "DeviceManager.h"
#include "hid/DeviceDiscovery.h"
#include "hid/Platform.h"
#include "storage/SettingsRepository.h"
#include <QClipboard>
#include <QDateTime>
#include <QGuiApplication>

namespace kcc {
static QString hex4(uint16_t v) { return QString("%1").arg(v, 4, 16, QChar('0')).toUpper(); }

DeviceManager::DeviceManager(QObject* parent) : QObject(parent), m_conn(new DeviceConnection) {
    m_writes = SettingsRepository().value("device/writesEnabled", false).toBool();
    m_conn->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_conn, &QObject::deleteLater);
    connect(m_conn, &DeviceConnection::log, this, &DeviceManager::appendLog);
    connect(m_conn, &DeviceConnection::opened, this, [this](bool ok, const QString& e) {
        m_ready = ok; m_connError = ok ? QString() : e;
        appendLog(ok ? "Config interface opened (0xFF13)" : "Config interface open failed: " + e);
        emit changed();
    });
    connect(m_conn, &DeviceConnection::finished, this, [this](const QString& l, bool ok, const QString& e) {
        m_lastResult = ok ? l + ": sent" : l + ": " + e;
        appendLog(ok ? "✓ " + l : "✗ " + l + ": " + e);
        end();
        emit transactionFinished(l, ok, e);
    });
    connect(m_conn, &DeviceConnection::perKeyColorsRead, this, [this](const QVariantMap& m, const QString& e) {
        m_lastResult = e.isEmpty() ? QString("Read %1 key colours").arg(m.size()) : e;
        end();
        emit perKeyColorsRead(m, e);
    });
    connect(m_conn, &DeviceConnection::descriptorRead, this, [this](const QString& t) {
        m_desc = t; emit descriptorChanged();
    });
    m_thread.start();
    m_scan.setInterval(1500);
    connect(&m_scan, &QTimer::timeout, this, &DeviceManager::rescan);
    m_scan.start();
    QTimer::singleShot(0, this, &DeviceManager::rescan);
}

DeviceManager::~DeviceManager() {
    QMetaObject::invokeMethod(m_conn, [c = m_conn] { c->close(); }, Qt::BlockingQueuedConnection);
    m_thread.quit();
    m_thread.wait();
}

void DeviceManager::appendLog(const QString& s) {
    m_log.prepend(QDateTime::currentDateTime().toString("HH:mm:ss.zzz  ") + s);
    while (m_log.size() > 1000) m_log.removeLast();
    emit logChanged();
}

void DeviceManager::rescan() {
    const auto found = DeviceDiscovery::scan();
    if (found.isEmpty()) {
        if (m_info) {
            m_info.reset(); m_ready = false;
            QMetaObject::invokeMethod(m_conn, [c = m_conn] { c->close(); });
            appendLog("Device disconnected");
            emit changed();
        }
        return;
    }
    if (m_info) return;   // already connected; hot-plug of a second unit is ignored
    m_info = found.first();
    appendLog(QString("Detected %1 %2:%3 rev %4, %5 HID collections")
                  .arg(m_info->model, hex4(m_info->vendorId), hex4(m_info->productId), revision())
                  .arg(m_info->interfaces.size()));
    if (const auto* c = m_info->configInterface()) {
        const QString path = c->path;
        QMetaObject::invokeMethod(m_conn, [cn = m_conn, path] { cn->open(path); });
    } else {
        m_connError = "Vendor configuration collection (0xFF13/0x0001) not found";
        appendLog(m_connError);
    }
    emit changed();
}

QString DeviceManager::vidPid() const {
    return m_info ? hex4(m_info->vendorId) + ":" + hex4(m_info->productId) : QString();
}
QString DeviceManager::revision() const {
    if (!m_info) return {};
    const uint16_t r = m_info->releaseBcd;
    return QString("%1.%2%3").arg(r >> 8, 0, 16).arg((r >> 4) & 0xF, 0, 16).arg(r & 0xF, 0, 16);
}
QString DeviceManager::platformName() const { return platform::name(); }

QVariantList DeviceManager::interfaces() const {
    QVariantList l;
    if (!m_info) return l;
    for (const auto& i : m_info->interfaces) {
        const char* role = i.role == InterfaceRole::Config ? "Configuration (vendor)"
                         : i.role == InterfaceRole::Keyboard ? "Keyboard"
                         : i.role == InterfaceRole::Consumer ? "Consumer control" : "Other";
        l << QVariantMap{{"role", role}, {"usagePage", "0x" + hex4(i.usagePage)}, {"usage", "0x" + hex4(i.usage)},
                         {"interface", i.interfaceNumber}, {"isConfig", i.role == InterfaceRole::Config}};
    }
    return l;
}

void DeviceManager::setWritesEnabled(bool on) {
    if (on == m_writes) return;
    m_writes = on;
    SettingsRepository().set("device/writesEnabled", on);
    appendLog(on ? "Device writes ENABLED by user" : "Device writes disabled");
    emit writesEnabledChanged();
}

bool DeviceManager::submit(const Transaction& tx, const QString& label, const QString& key) {
    if (!m_writes) { appendLog("Refused (writes disabled): " + label); return false; }
    if (!m_ready) { appendLog("Refused (not connected): " + label); return false; }
    if (key.isEmpty()) {
        begin();
        QMetaObject::invokeMethod(m_conn, [c = m_conn, tx, label] { c->run(tx, label); });
        return true;
    }
    auto& slot = m_slots[key];
    if (!slot) slot = std::make_shared<Slot>();
    bool queued;
    {
        QMutexLocker l(&slot->m);
        queued = slot->job.has_value();
        slot->job = std::make_pair(tx, label);
    }
    if (queued) return true;   // the pending worker call will pick up this newer state
    begin();
    QMetaObject::invokeMethod(m_conn, [c = m_conn, s = slot] {
        std::pair<Transaction, QString> job;
        {
            QMutexLocker l(&s->m);
            job = std::move(*s->job);
            s->job.reset();
        }
        c->run(job.first, job.second);
    });
    return true;
}

void DeviceManager::readPerKeyColors() {
    // Read-only is fine: 04 F5 + 04 02 is the vendor app's own read sequence and changes no setting.
    if (!m_ready) { appendLog("Refused (not connected): read per-key colours"); return; }
    begin();
    QMetaObject::invokeMethod(m_conn, [c = m_conn] { c->readPerKeyColors(); });
}

void DeviceManager::inspectInterface(int index) {
    if (!m_info || index < 0 || index >= m_info->interfaces.size()) return;
    const QString path = m_info->interfaces[index].path;
    QMetaObject::invokeMethod(m_conn, [c = m_conn, path] { c->readDescriptor(path); });
}

void DeviceManager::copyLog() const {
    if (auto* cb = QGuiApplication::clipboard()) cb->setText(m_log.join('\n'));
}
}
