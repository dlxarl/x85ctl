import QtQuick

// Toggle chip (modifier keys).
Rectangle {
    id: chip
    property string text
    property bool checked: false
    implicitWidth: t.implicitWidth + 20; implicitHeight: 26; radius: 13
    color: checked ? Qt.alpha(Theme.accent, 0.18) : ma.containsMouse ? Theme.hover : Theme.raised
    border.color: checked ? Theme.accent : Theme.line
    Text { id: t; anchors.centerIn: parent; text: chip.text; font.pixelSize: 12
           color: chip.checked ? Theme.text : Theme.dim; font.weight: chip.checked ? Font.DemiBold : Font.Normal }
    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: chip.checked = !chip.checked }
}
