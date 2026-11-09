import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Workspace {
    id: page
    title: "Device"
    property string view: "traffic"

    SectionLabel { text: "Keyboard access" }
    AppSwitch { text: "Allow writing to the keyboard"; checked: Device.writesEnabled; onToggled: Device.writesEnabled = checked }
    Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.dim; font.pixelSize: 12; lineHeight: 1.2
           text: "Packets match the vendor app byte for byte (decompiled with Ghidra). Nothing is sent while this is off. Firmware, bootloader and reset commands are never sent." }

    SectionLabel { text: "Background" }
    Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.dim; font.pixelSize: 12; lineHeight: 1.3
           text: "Closing the window keeps the app running (shortcuts, Snake, keyboard link). Reopen it from the menu-bar icon, or by opening the app again from Launchpad or Finder." }
    AppSwitch { text: "Hide the Dock icon while the window is closed"; checked: Shell.hideDockInBackground
                enabled: Shell.trayAvailable; onToggled: Shell.hideDockInBackground = checked }
    Text { visible: Shell.hideDockInBackground; Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.warn; font.pixelSize: 11
           text: "If your menu-bar manager hides the icon, turn this off to keep the Dock icon." }
    SectionLabel { text: "Identity" }
    Repeater {
        model: [["Model", Device.model || "—"], ["Manufacturer", Device.manufacturer || "—"],
                ["VID:PID", Device.vidPid || "—"], ["Firmware rev", Device.revision || "—"],
                ["Firmware version", "not readable (no such command)"],
                ["Config interface", Device.ready ? "open · 0xFF13/0x0001" : (Device.connectionError || "—")],
                ["Platform", Device.platformName]]
        delegate: RowLayout {
            Layout.fillWidth: true
            Text { text: modelData[0]; color: Theme.faint; font.pixelSize: 12; Layout.preferredWidth: 110 }
            Text { text: modelData[1]; color: Theme.text; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.Wrap }
        }
    }

    SectionLabel { text: "HID collections" }
    Repeater {
        model: Device.interfaces
        delegate: Rectangle {
            Layout.fillWidth: true; height: 40; radius: Theme.radius
            color: dma.containsMouse ? Theme.hover : Theme.raised
            Column { anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter } spacing: 1
                Text { text: modelData.role; color: modelData.isConfig ? Theme.accent : Theme.text; font.pixelSize: 12 }
                Text { text: "iface " + modelData.interface + " · " + modelData.usagePage + "/" + modelData.usage
                       color: Theme.faint; font.family: Theme.mono; font.pixelSize: 11 } }
            MouseArea { id: dma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: { Device.inspectInterface(index); page.view = "descriptor" } }
        }
    }
    Text { visible: !Device.connected; text: "No device."; color: Theme.dim; font.pixelSize: 12 }

    stage: ColumnLayout {
        anchors.fill: parent; spacing: 10
        RowLayout {
            Layout.fillWidth: true; spacing: 10
            Segmented { model: [{label: "HID traffic", value: "traffic"}, {label: "Report descriptor", value: "descriptor"}]
                        value: page.view; onActivated: (v) => page.view = v }
            Item { Layout.fillWidth: true }
            AppButton { visible: page.view === "traffic"; text: "Copy"; onClicked: Device.copyLog() }
            AppButton { visible: page.view === "traffic"; text: "Clear"; onClicked: Device.clearLog() }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; radius: 8; color: Theme.surface; border.color: Theme.line
            ScrollView {
                anchors.fill: parent; anchors.margins: 12
                TextArea {
                    readOnly: true; selectByMouse: true; wrapMode: TextArea.WrapAnywhere; background: null
                    font.family: Theme.mono; font.pixelSize: 12; color: Theme.text
                    text: page.view === "traffic" ? (Device.log.join("\n") || "No traffic yet.")
                                                  : (Device.descriptorText || "Click a HID collection on the left (read-only).")
                }
            }
        }
    }
}
