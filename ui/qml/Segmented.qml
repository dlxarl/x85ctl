import QtQuick
import QtQuick.Layouts

// Segmented control. model: [{label, value, enabled?}]
Rectangle {
    id: seg
    property var model: []
    property var value
    signal activated(var value)
    implicitHeight: 30
    implicitWidth: row.implicitWidth + 6
    radius: 8; color: Theme.raised; border.color: Theme.line
    RowLayout {
        id: row; anchors.fill: parent; anchors.margins: 3; spacing: 2
        Repeater {
            model: seg.model
            delegate: Rectangle {
                readonly property bool on: modelData.value === seg.value
                readonly property bool usable: modelData.enabled === undefined || modelData.enabled
                Layout.fillHeight: true; Layout.fillWidth: true
                implicitWidth: content.implicitWidth + 20
                radius: 6; color: on ? Theme.surface : "transparent"
                border.color: on ? Theme.line : "transparent"
                Row {
                    id: content; anchors.centerIn: parent; spacing: 6
                    Icon { visible: !!modelData.icon; name: modelData.icon || ""; size: 13
                           anchors.verticalCenter: parent.verticalCenter
                           color: !usable ? Theme.faint : on ? Theme.text : Theme.dim }
                    Text { id: t; text: modelData.label; font.pixelSize: Theme.fontSmall
                           anchors.verticalCenter: parent.verticalCenter
                           font.weight: on ? Font.DemiBold : Font.Normal
                           color: !usable ? Theme.faint : on ? Theme.text : Theme.dim }
                }
                TapHandler { enabled: usable; onTapped: seg.activated(modelData.value) }
            }
        }
    }
}
