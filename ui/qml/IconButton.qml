import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: b
    property string glyph
    property string tip
    signal clicked()
    width: 28; height: 28; radius: 5
    color: ma.containsMouse && enabled ? Theme.hover : "transparent"
    opacity: enabled ? 1 : 0.35
    Text { anchors.centerIn: parent; text: b.glyph; color: Theme.dim; font.pixelSize: 13 }
    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; enabled: b.enabled
                cursorShape: Qt.PointingHandCursor; onClicked: b.clicked() }
    ToolTip.visible: ma.containsMouse && tip !== ""; ToolTip.text: tip; ToolTip.delay: 500
}
