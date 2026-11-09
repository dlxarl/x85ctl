#pragma once
#include "protocol/CommandBuilder.h"
#include <QJsonArray>
#include <QObject>
#include <QVariantList>

namespace kcc {
class DeviceManager;

// Macros stored on the host (in the profile) and uploaded with cmd 0x15 (docs/PROTOCOL.md §8).
// A key is bound to a macro by its position in this list, so the list is always uploaded as a whole.
class MacroService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList macros READ macros NOTIFY changed)       // [{name, steps, empty}]
    Q_PROPERTY(int bytesUsed READ bytesUsed NOTIFY changed)
    Q_PROPERTY(int bytesMax READ bytesMax CONSTANT)
    Q_PROPERTY(QString error READ error NOTIFY changed)               // why an upload would be refused
public:
    explicit MacroService(DeviceManager* dev, QObject* parent = nullptr) : QObject(parent), m_dev(dev) {}
    QVariantList macros() const;
    int bytesUsed() const { return CommandBuilder::macroBytes(m_list); }
    int bytesMax() const { return CommandBuilder::kMacroBufferBytes - x85::kPacketSize; }   // last page is reserved by the vendor layout
    QString error() const;
    const QVector<MacroDef>& list() const { return m_list; }
    QString nameOf(int index) const { return index >= 0 && index < m_list.size() ? m_list[index].name : QString(); }

    Q_INVOKABLE int add(const QString& name);                       // returns the new index
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void rename(int index, const QString& name);
    Q_INVOKABLE QVariantList steps(int index) const;                // [{type, value, label}]
    // type: "key" (down+up, with optional modifier bitmask 1 Ctrl 2 Shift 4 Alt 8 Win), "keydown", "keyup",
    //       "mouse" (click), "mousedown", "mouseup", "delay"
    // at < 0 appends; otherwise the new event(s) are inserted before step `at`. Returns how many were added.
    Q_INVOKABLE int addStep(int index, const QString& type, int value, int modifiers = 0, int at = -1);
    Q_INVOKABLE void setStepValue(int index, int step, int value);   // delay ms / key usage / mouse button
    Q_INVOKABLE void removeStep(int index, int step);
    Q_INVOKABLE void moveStep(int index, int step, int delta);
    Q_INVOKABLE void clearSteps(int index);
    Q_INVOKABLE void setAllDelays(int index, int ms);   // ms <= 0 removes every delay step
    Q_INVOKABLE int totalMs(int index) const;
    Q_INVOKABLE bool apply();                                       // upload the whole table

    QJsonArray toJson() const;
    void fromJson(const QJsonArray& a);
signals:
    void changed();
    void removed(int index);    // key bindings above this index must shift
private:
    DeviceManager* m_dev;
    QVector<MacroDef> m_list;
};
}
