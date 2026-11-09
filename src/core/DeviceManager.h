#pragma once
#include "DeviceConnection.h"
#include "DeviceInfo.h"
#include <QObject>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QHash>
#include <QMutex>
#include <QVariantList>
#include <memory>
#include <optional>

namespace kcc {
class DeviceManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(bool ready READ ready NOTIFY changed)            // config interface open
    Q_PROPERTY(QString model READ model NOTIFY changed)
    Q_PROPERTY(QString vidPid READ vidPid NOTIFY changed)
    Q_PROPERTY(QString revision READ revision NOTIFY changed)
    Q_PROPERTY(QString manufacturer READ manufacturer NOTIFY changed)
    Q_PROPERTY(QString platformName READ platformName CONSTANT)
    Q_PROPERTY(QVariantList interfaces READ interfaces NOTIFY changed)
    Q_PROPERTY(QString connectionError READ connectionError NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool writesEnabled READ writesEnabled WRITE setWritesEnabled NOTIFY writesEnabledChanged)
    Q_PROPERTY(QString descriptorText READ descriptorText NOTIFY descriptorChanged)
    Q_PROPERTY(QStringList log READ log NOTIFY logChanged)
    Q_PROPERTY(QString lastResult READ lastResult NOTIFY busyChanged)
public:
    explicit DeviceManager(QObject* parent = nullptr);
    ~DeviceManager() override;

    bool connected() const { return m_info.has_value(); }
    bool ready() const { return m_ready; }
    QString model() const { return m_info ? m_info->model : QString(); }
    QString vidPid() const;
    QString revision() const;
    QString manufacturer() const { return m_info ? m_info->manufacturer : QString(); }
    QString platformName() const;
    QVariantList interfaces() const;
    QString connectionError() const { return m_connError; }
    bool busy() const { return m_pending > 0; }
    bool writesEnabled() const { return m_writes; }
    void setWritesEnabled(bool on);
    QString descriptorText() const { return m_desc; }
    QStringList log() const { return m_log; }
    QString lastResult() const { return m_lastResult; }

    // Queues a transaction on the worker. Refused unless writes are enabled and the device is ready.
    // With a coalesce key, a newer submit replaces a queued (not yet started) one with the same key,
    // so dragging a slider sends only the latest state instead of a backlog of stale ones.
    bool submit(const Transaction& tx, const QString& label, const QString& coalesceKey = {});

    Q_INVOKABLE void rescan();
    Q_INVOKABLE void inspectInterface(int index);
    Q_INVOKABLE void readPerKeyColors();
    Q_INVOKABLE void clearLog() { m_log.clear(); emit logChanged(); }
    Q_INVOKABLE void copyLog() const;

signals:
    void changed();
    void busyChanged();
    void writesEnabledChanged();
    void descriptorChanged();
    void logChanged();
    void transactionFinished(const QString& label, bool ok, const QString& error);
    void perKeyColorsRead(const QVariantMap& byLightIndex, const QString& error);

private:
    void appendLog(const QString& s);
    void begin() { ++m_pending; emit busyChanged(); }
    void end() { m_pending = qMax(0, m_pending - 1); emit busyChanged(); }

    struct Slot { QMutex m; std::optional<std::pair<Transaction, QString>> job; };
    QHash<QString, std::shared_ptr<Slot>> m_slots;
    QThread m_thread;
    DeviceConnection* m_conn;   // owned by m_thread
    QTimer m_scan;
    std::optional<DeviceInfo> m_info;
    bool m_ready = false, m_writes = false;
    int m_pending = 0;
    QString m_connError, m_desc, m_lastResult;
    QStringList m_log;
};
}
