import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

Workspace {
    id: page
    title: "Profiles"
    hint: "Profile “" + ProfileStore.active + "” — lighting and both key layers, stored on this computer"

    stage: KeyboardStage {
        anchors.fill: parent
        lightMode: Lighting.perKeyActive ? "perkey" : "effect"
        keyBadge: function(k) { const l = KeyMap.labels[String(k.keyIndex)]; return l !== undefined ? l : "" }
    }

    SectionLabel { text: "Saved on this computer" }
    ColumnLayout {
        Layout.fillWidth: true; Layout.minimumWidth: 0; spacing: 4
        Repeater {
            model: ProfileStore.profiles
            delegate: Rectangle {
                readonly property bool on: modelData === ProfileStore.active
                Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 44; radius: Theme.radius
                color: on ? Qt.alpha(Theme.accent, 0.14) : pma.containsMouse ? Theme.hover : Theme.raised
                border.color: on ? Theme.accent : "transparent"
                MouseArea { id: pma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: ProfileStore.activate(modelData) }
                RowLayout {
                    anchors { fill: parent; leftMargin: 12; rightMargin: 6 } spacing: 4
                    Text { text: modelData; color: Theme.text; font.pixelSize: 13; font.weight: on ? Font.DemiBold : Font.Normal
                           Layout.fillWidth: true; Layout.minimumWidth: 0; elide: Text.ElideRight }
                    Text { visible: on; text: "ACTIVE"; color: Theme.accent; font.pixelSize: 10; font.letterSpacing: 1; font.weight: Font.DemiBold }
                    IconButton { glyph: "↺"; tip: "Overwrite with the current settings"; onClicked: ProfileStore.overwrite(modelData) }
                    IconButton { glyph: "⇪"; tip: "Export"; onClicked: { exportDlg.profile = modelData; exportDlg.open() } }
                    IconButton { glyph: "✕"; tip: "Delete"; enabled: ProfileStore.profiles.length > 1
                                 onClicked: { confirm.profile = modelData; confirm.open() } }
                }
            }
        }
    }
    SectionLabel { text: "Save as new profile" }
    RowLayout {
        Layout.fillWidth: true; Layout.minimumWidth: 0; spacing: 8
        TextField { id: nameField; Layout.fillWidth: true; Layout.minimumWidth: 0; implicitWidth: 60; placeholderText: "Name"; color: Theme.text; placeholderTextColor: Theme.faint
                    font.pixelSize: 13; leftPadding: 10
                    background: Rectangle { radius: Theme.radius; color: Theme.bg; border.color: nameField.activeFocus ? Theme.accent : Theme.line }
                    onAccepted: if (ProfileStore.create(text)) clear() }
        AppButton { text: "Save as new"; onClicked: if (ProfileStore.create(nameField.text)) nameField.clear() }
    }
    AppButton { text: "Import from file…"; Layout.fillWidth: true; onClicked: importDlg.open() }
    Text { visible: ProfileStore.message !== ""; text: ProfileStore.message; color: Theme.dim; font.pixelSize: 11
           Layout.fillWidth: true; wrapMode: Text.Wrap }

    footer: [
        RowLayout {
            Layout.fillWidth: true; Layout.minimumWidth: 0; spacing: 10
            AppButton { text: "Save"; onClicked: ProfileStore.saveActive() }
            Item { Layout.fillWidth: true }
            AppButton { text: "Send to keyboard"; primary: true; Layout.minimumWidth: 0; enabled: Device.ready && Device.writesEnabled
                        onClicked: ProfileStore.applyToKeyboard() }
        },
        WriteGate { Layout.fillWidth: true }
    ]

    FileDialog { id: importDlg; nameFilters: ["Profile JSON (*.json)"]; onAccepted: ProfileStore.importProfile(selectedFile) }
    FileDialog { id: exportDlg; property string profile; fileMode: FileDialog.SaveFile; defaultSuffix: "json"
                 nameFilters: ["Profile JSON (*.json)"]; onAccepted: ProfileStore.exportProfile(profile, selectedFile) }
    MessageDialog { id: confirm; property string profile; text: "Delete profile “" + profile + "”?"
                    informativeText: "This removes it from this computer. It cannot be undone."
                    buttons: MessageDialog.Ok | MessageDialog.Cancel
                    onAccepted: ProfileStore.remove(profile) }
}
