#pragma once
#include "hid/HidTransport.h"
#include "protocol/CommandBuilder.h"
#include <QByteArray>
#include <functional>

namespace kcc {
// Executes vendor-style transactions with the vendor app's pacing (FUN_0044faa0/0044fc90/0044ff70).
class TransactionRunner {
public:
    using SleepFn = std::function<void(int ms)>;
    using LogFn = std::function<void(const QString&)>;

    TransactionRunner(HidTransport& t, SleepFn sleep, LogFn log, int delayMs = x85::kCmdDelayMs)
        : m_t(t), m_sleep(std::move(sleep)), m_log(std::move(log)), m_delay(delayMs) {}

    struct Result { bool ok = true; QString error; };
    Result run(const Transaction& tx);
    // Read-back of the per-key colour table (cmd 0xF5). Returns 576 bytes or empty on failure.
    QByteArray readPerKeyColors(QString* error = nullptr);

private:
    bool send(const QByteArray& p);
    QByteArray get();
    HidTransport& m_t;
    SleepFn m_sleep;
    LogFn m_log;
    int m_delay;
};
}
