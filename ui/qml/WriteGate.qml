import QtQuick
import QtQuick.Layouts

// Inline status line explaining why "Apply" is unavailable.
RowLayout {
    readonly property bool canWrite: Device.ready && Device.writesEnabled
    visible: !canWrite || Device.busy || Device.lastResult !== ""
    spacing: 8
    Rectangle { width: 6; height: 6; radius: 3
                color: !Device.ready ? Theme.faint : !Device.writesEnabled ? Theme.warn : Device.busy ? Theme.accent : Theme.ok }
    Text {
        Layout.fillWidth: true; wrapMode: Text.Wrap; font.pixelSize: Theme.fontSmall; color: Theme.dim
        text: !Device.ready ? "Keyboard not connected."
            : !Device.writesEnabled ? "Read-only mode. Turn on “Writing to keyboard” in the top bar to send changes."
            : Device.busy ? "Sending…" : Device.lastResult
    }
}
