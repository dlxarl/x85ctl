import QtQuick
import QtQuick.Controls.Basic

Button {
    id: b
    property bool primary: false
    property bool danger: false
    implicitHeight: 32
    leftPadding: 14; rightPadding: 14
    font.pixelSize: Theme.fontBody
    contentItem: Text {
        text: b.text; font: b.font; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        color: !b.enabled ? Theme.faint : b.primary ? Theme.accentInk : b.danger ? Theme.danger : Theme.text
    }
    background: Rectangle {
        radius: 7
        color: b.primary ? (b.enabled ? (b.down ? Qt.darker(Theme.accent, 1.15) : Theme.accent) : Theme.line)
                         : (b.down ? Theme.line : b.hovered ? Theme.raised : Theme.surface)
        border.color: b.primary ? "transparent" : Theme.line
        Behavior on color { ColorAnimation { duration: 100 } }
    }
}
