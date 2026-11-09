import QtQuick
import QtQuick.Controls.Basic

Switch {
    id: c
    font.pixelSize: Theme.fontBody
    indicator: Rectangle {
        implicitWidth: 34; implicitHeight: 20; radius: 10
        x: c.leftPadding; y: parent.height / 2 - height / 2
        color: c.checked ? Theme.accent : Theme.line
        Behavior on color { ColorAnimation { duration: 120 } }
        Rectangle { width: 16; height: 16; radius: 8; y: 2; x: c.checked ? parent.width - width - 2 : 2; color: "white"
                    Behavior on x { NumberAnimation { duration: 120 } } }
    }
    contentItem: Text { leftPadding: c.indicator.width + 10; text: c.text; font: c.font; color: c.enabled ? Theme.text : Theme.faint
                        verticalAlignment: Text.AlignVCenter }
}
