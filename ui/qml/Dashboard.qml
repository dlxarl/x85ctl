import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Home: the device front and centre, with shortcuts to the editors.
Item {
    id: home
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 36; spacing: 18
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter; spacing: 4
            Text { Layout.alignment: Qt.AlignHCenter; text: "ATTACK SHARK X85"; color: Theme.text
                   font.pixelSize: 22; font.weight: Font.Bold; font.letterSpacing: 2 }
            Text { Layout.alignment: Qt.AlignHCenter; color: Theme.dim; font.pixelSize: 12
                   text: Device.connected ? "USB " + Device.vidPid + " · rev " + Device.revision + " · profile " + ProfileStore.active
                                          : "Connect the keyboard with a USB cable (2.4 GHz and Bluetooth are not supported)" }
        }
        KeyboardStage {
            Layout.fillWidth: true; Layout.fillHeight: true
            lightMode: Lighting.perKeyActive ? "perkey" : "effect"
            opacity: Device.connected ? 1 : 0.45
            onKeyClicked: win.go("Lighting")
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter; spacing: 14
            Repeater {
                model: [
                    { t: "Lighting", s: Lighting.perKeyActive ? "Per-key colours" : (Lighting.modes.find(m => m.id === Lighting.mode) || {name: "—"}).name, go: "Lighting" },
                    { t: "Assignments", s: Object.keys(KeyMap.labels).length + " keys changed", go: "Assignments" },
                    { t: "Profiles", s: ProfileStore.profiles.length + " stored", go: "Profiles" },
                    { t: "Device", s: Device.writesEnabled ? "Writing enabled" : "Read-only", go: "Device" }
                ]
                delegate: Rectangle {
                    width: 200; height: 64; radius: 8
                    color: hma.containsMouse ? Theme.hover : Theme.surface; border.color: Theme.line
                    Column { anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter } spacing: 4
                        Text { text: modelData.t; color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold
                               font.capitalization: Font.AllUppercase; font.letterSpacing: 1 }
                        Text { text: modelData.s; color: Theme.dim; font.pixelSize: 12 } }
                    Text { anchors { right: parent.right; rightMargin: 14; verticalCenter: parent.verticalCenter }
                           text: "›"; color: Theme.faint; font.pixelSize: 20 }
                    MouseArea { id: hma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: win.go(modelData.go) }
                }
            }
        }
    }
}
