import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

RowLayout {
    id: r
    property string label
    property alias from: s.from
    property alias to: s.to
    property alias value: s.value
    property alias stepSize: s.stepSize
    property int decimals: 0
    property string suffix: ""
    signal moved(real value)
    spacing: 12
    Text { text: r.label; color: Theme.dim; font.pixelSize: Theme.fontBody; Layout.preferredWidth: 84 }
    Slider {
        id: s; Layout.fillWidth: true; snapMode: Slider.SnapAlways; stepSize: 1
        onMoved: r.moved(value)
        background: Rectangle {
            x: s.leftPadding; y: s.topPadding + s.availableHeight / 2 - height / 2
            width: s.availableWidth; height: 4; radius: 2; color: Theme.line
            Rectangle { width: s.visualPosition * parent.width; height: parent.height; radius: 2; color: Theme.accent }
        }
        handle: Rectangle {
            x: s.leftPadding + s.visualPosition * (s.availableWidth - width)
            y: s.topPadding + s.availableHeight / 2 - height / 2
            width: 16; height: 16; radius: 8; color: Theme.surface; border.color: Theme.accent; border.width: 2
        }
    }
    Text { text: s.value.toFixed(r.decimals) + r.suffix; color: Theme.text; font.pixelSize: Theme.fontBody; font.family: Theme.mono
           Layout.preferredWidth: r.suffix || r.decimals ? 40 : 22; horizontalAlignment: Text.AlignRight }
}
